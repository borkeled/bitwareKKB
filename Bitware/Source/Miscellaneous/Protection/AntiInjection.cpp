#include "AntiInjection.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <winternl.h>
#include <wintrust.h>
#include <SoftPub.h>
#include <Auth/skStr.h>
#include <Infrastructure/ApiHiding.h>
#include <Infrastructure/Logger.h>

#pragma comment(lib, "wintrust.lib")

static LONG DynamicWinVerifyTrust(HWND hwnd, GUID* pgActionID, void* pWVTData) {
    static auto fn = (LONG(WINAPI*)(HWND, GUID*, void*))::GetProcAddress(
        ::LoadLibraryW(L"wintrust.dll"), "WinVerifyTrust");
    if (fn) return fn(hwnd, pgActionID, pWVTData);
    return -1;
}

typedef struct _LDR_DLL_NOTIFICATION_DATA {
    ULONG Flags;
    PCUNICODE_STRING FullDllName;
    PCUNICODE_STRING BaseDllName;
    PVOID DllBase;
    ULONG SizeOfImage;
} LDR_DLL_NOTIFICATION_DATA, *PLDR_DLL_NOTIFICATION_DATA;
typedef const LDR_DLL_NOTIFICATION_DATA* PCLDR_DLL_NOTIFICATION_DATA;

#define LDR_DLL_NOTIFICATION_REASON_LOADED 1

typedef VOID(NTAPI* PLDR_DLL_NOTIFICATION_FUNCTION)(ULONG, PCLDR_DLL_NOTIFICATION_DATA, PVOID);

HMODULE AntiInjection::baselineModules[256];
int AntiInjection::baselineModuleCount = 0;
HANDLE AntiInjection::hThread = nullptr;
bool AntiInjection::running = false;
PVOID AntiInjection::dllNotifyCookie = nullptr;

static int GetModules(HMODULE* buffer, int maxCount)
{
    DWORD needed = 0;
    if (Api::EnumProcessModules(Api::GetCurrentProcess(), buffer, maxCount * sizeof(HMODULE), &needed))
    {
        int count = static_cast<int>(needed / sizeof(HMODULE));
        if (count > maxCount) count = maxCount;
        return count;
    }
    return 0;
}

void AntiInjection::Start()
{
    if (running) return;
    baselineModuleCount = GetModules(baselineModules, 256);
    running = true;
    hThread = Api::CreateThread(nullptr, 0, MonitorThread, nullptr, 0, nullptr);
}

static VOID NTAPI DllNotificationCallback(ULONG Reason, PCLDR_DLL_NOTIFICATION_DATA Data, PVOID Ctx)
{
    if (Reason == LDR_DLL_NOTIFICATION_REASON_LOADED)
    {
        auto mod = Data->BaseDllName;
        if (!mod->Buffer) return;

        Api::CharLowerBuffW(mod->Buffer, mod->Length / sizeof(wchar_t));

        if (mod->Length >= 22 && wcsstr(mod->Buffer, L"ntdll.dll") != nullptr)
            return;
        if (mod->Length >= 10 && wcsstr(mod->Buffer, L"kernel32.dll") != nullptr)
            return;
        if (mod->Length >= 10 && wcsstr(mod->Buffer, L"kernelbase.dll") != nullptr)
            return;

        HMODULE hMod = (HMODULE)Data->DllBase;
        wchar_t fullPath[MAX_PATH];
        if (Api::GetModuleFileNameExW(Api::GetCurrentProcess(), hMod, fullPath, MAX_PATH))
        {
            Api::CharLowerBuffW(fullPath, wcslen(fullPath));

            WINTRUST_FILE_INFO FileInfo;
            memset(&FileInfo, 0, sizeof(FileInfo));
            FileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
            FileInfo.pcwszFilePath = fullPath;
            FileInfo.hFile = NULL;

            GUID guidAction = WINTRUST_ACTION_GENERIC_VERIFY_V2;

            WINTRUST_DATA WvtData;
            memset(&WvtData, 0, sizeof(WvtData));
            WvtData.cbStruct = sizeof(WvtData);
            WvtData.dwUnionChoice = WTD_CHOICE_FILE;
            WvtData.pFile = &FileInfo;
            WvtData.dwUIChoice = WTD_UI_NONE;
            WvtData.fdwRevocationChecks = WTD_REVOKE_NONE;
            WvtData.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL;
            WvtData.dwStateAction = WTD_STATEACTION_IGNORE;

            LONG status = DynamicWinVerifyTrust(NULL, &guidAction, &WvtData);
            if (status != ERROR_SUCCESS)
            {
                *reinterpret_cast<volatile bool*>(Ctx) = true;
            }
        }
    }
}

void AntiInjection::Stop()
{
    if (dllNotifyCookie)
    {
        Api::LdrUnregisterDllNotification(dllNotifyCookie);
        dllNotifyCookie = nullptr;
    }

    running = false;
    if (hThread)
    {
        Api::WaitForSingleObject(hThread, INFINITE);
        Api::CloseHandle(hThread);
        hThread = nullptr;
    }
}

DWORD __stdcall AntiInjection::MonitorThread(void*)
{
    static volatile bool dllDetected = false;

    PLDR_DLL_NOTIFICATION_FUNCTION notifyFn = DllNotificationCallback;
    Api::LdrRegisterDllNotification(0, (PVOID)notifyFn, const_cast<bool*>(&dllDetected), &dllNotifyCookie);

    int scanCounter = 0;

    while (running)
    {
        bool detected = false;

        if (dllDetected)
        {
            Logger::Log("[AntiInjection] unsigned module detected via DLL notification");
            detected = true;
        }

        if (DetectDebugger())
        {
            Logger::Log("[AntiInjection] debugger detected (IsDebuggerPresent)");
            detected = true;
        }

        if (DetectRemoteDebugger())
        {
            Logger::Log("[AntiInjection] remote debugger detected");
            detected = true;
        }

        ++scanCounter;

        if (scanCounter % 100 == 0)
        {
            if (DetectThreadStartAnomalies())
            {
                Logger::Log("[AntiInjection] thread start anomaly detected");
                detected = true;
            }
        }

        if (scanCounter % 600 == 0)
        {
            if (DetectManualMapRegions())
            {
                Logger::Log("[AntiInjection] manual map region detected");
                detected = true;
            }
        }

        if (scanCounter % 600 == 300)
        {
            if (DetectNtdllHooks())
            {
                Logger::Log("[AntiInjection] ntdll hooks detected");
                detected = true;
            }
        }

        if (detected)
        {
            Api::TerminateProcess(Api::GetCurrentProcess(), 0);
        }

        Api::Sleep(100);
    }
    return 0;
}

bool AntiInjection::DetectDebugger() { return Api::IsDebuggerPresent(); }
bool AntiInjection::DetectRemoteDebugger() { BOOL present = FALSE; Api::CheckRemoteDebuggerPresent(Api::GetCurrentProcess(), &present); return present; }

bool AntiInjection::DetectNewModules()
{
    HMODULE now[256];
    int nowCount = GetModules(now, 256);

    for (int i = 0; i < nowCount; i++)
    {
        bool known = false;
        for (int j = 0; j < baselineModuleCount; j++)
        {
            if (now[i] == baselineModules[j]) { known = true; break; }
        }

        if (!known)
        {
            return true;
        }
    }
    return false;
}

bool AntiInjection::DetectManualMapRegions()
{
    HMODULE buffer[256];
    int knownCount = GetModules(buffer, 256);
    uintptr_t knownBases[256];
    for (int i = 0; i < knownCount; i++)
        knownBases[i] = (uintptr_t)buffer[i];

    SYSTEM_INFO si;
    Api::GetSystemInfo(&si);
    uintptr_t addr = (uintptr_t)si.lpMinimumApplicationAddress;

    while (addr < (uintptr_t)si.lpMaximumApplicationAddress)
    {
        MEMORY_BASIC_INFORMATION mbi;
        if (Api::VirtualQuery((const void*)addr, &mbi, sizeof(mbi)) == 0)
        {
            break;
        }

        if (mbi.State == MEM_COMMIT && mbi.Type == MEM_IMAGE)
        {
            bool found = false;
            for (int i = 0; i < knownCount; i++)
            {
                if ((uintptr_t)mbi.AllocationBase == knownBases[i])
                {
                    found = true;
                    break;
                }
            }
            if (!found) return true;
        }

        addr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
    return false;
}

bool AntiInjection::DetectThreadStartAnomalies()
{
    HANDLE snap = Api::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    THREADENTRY32 te;
    te.dwSize = sizeof(te);
    if (!Api::Thread32First(snap, &te))
    {
        Api::CloseHandle(snap);
        return false;
    }

    DWORD pid = Api::GetCurrentProcessId();
    do
    {
        if (te.th32OwnerProcessID != pid) continue;

        HANDLE thread = Api::OpenThread(THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
        if (!thread) continue;

        PVOID startAddr = NULL;
        NTSTATUS status = Api::NtQueryInformationThread(thread, (THREADINFOCLASS)9, &startAddr, sizeof(startAddr), NULL);
        if (NT_SUCCESS(status) && startAddr)
        {
            bool found = false;
            HMODULE buffer[256];
            int modCount = GetModules(buffer, 256);
            for (int i = 0; i < modCount; i++)
            {
                MODULEINFO mi;
                if (Api::GetModuleInformation(Api::GetCurrentProcess(), buffer[i], &mi, sizeof(mi)))
                {
                    uintptr_t start = (uintptr_t)startAddr;
                    uintptr_t modBase = (uintptr_t)mi.lpBaseOfDll;
                    if (start >= modBase && start < modBase + mi.SizeOfImage)
                    {
                        found = true;
                        break;
                    }
                }
            }
            if (!found)
            {
                Api::CloseHandle(thread);
                Api::CloseHandle(snap);
                return true;
            }
        }
        Api::CloseHandle(thread);
    } while (Api::Thread32Next(snap, &te));

    Api::CloseHandle(snap);
    return false;
}

static bool CompareExportPrologue(HMODULE ntdll, void* diskImage, const char* exportName, uint8_t diskBuf[12], uint8_t memBuf[12])
{
    auto dos = (PIMAGE_DOS_HEADER)diskImage;
    auto nt = (PIMAGE_NT_HEADERS)((uintptr_t)diskImage + dos->e_lfanew);
    auto dataDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!dataDir.VirtualAddress || !dataDir.Size) return false;

    auto exportBase = (uintptr_t)diskImage;
    auto exp = (PIMAGE_EXPORT_DIRECTORY)(exportBase + dataDir.VirtualAddress);

    auto names = (DWORD*)(exportBase + exp->AddressOfNames);
    auto funcs = (DWORD*)(exportBase + exp->AddressOfFunctions);
    auto ords = (WORD*)(exportBase + exp->AddressOfNameOrdinals);

    for (DWORD i = 0; i < exp->NumberOfNames; i++)
    {
        const char* currentName = (const char*)(exportBase + names[i]);
        if (strcmp(currentName, exportName) != 0) continue;

        DWORD funcRva = funcs[ords[i]];
        uint8_t* diskAddr = (uint8_t*)(exportBase + funcRva);
        uint8_t* memAddr = (uint8_t*)((uintptr_t)ntdll + funcRva);

        memcpy(diskBuf, diskAddr, 12);
        memcpy(memBuf, memAddr, 12);

        return (memcmp(diskBuf, memBuf, 12) == 0);
    }
    return true;
}

bool AntiInjection::DetectNtdllHooks()
{
    HMODULE ntdll = Api::GetModuleHandleW(skCrypt(L"ntdll.dll"));
    if (!ntdll) return false;

    wchar_t path[MAX_PATH];
    if (!Api::GetModuleFileNameExW(Api::GetCurrentProcess(), ntdll, path, MAX_PATH))
        return false;

    HANDLE file = Api::CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;

    HANDLE mapping = Api::CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!mapping)
    {
        Api::CloseHandle(file);
        return false;
    }

    void* diskImage = Api::MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (!diskImage)
    {
        Api::CloseHandle(mapping);
        Api::CloseHandle(file);
        return false;
    }

    const char* targetExports[] = {
        "NtMapViewOfSection",
        "NtWriteVirtualMemory",
        "NtCreateThreadEx",
        "NtQueryInformationProcess",
        "NtOpenProcess",
        "NtOpenThread",
        "NtResumeThread",
        "NtClose",
        "NtReadVirtualMemory",
        "NtProtectVirtualMemory",
        "NtCreateProcessEx",
        "NtQueueApcThread"
    };

    bool hooked = false;
    for (auto expName : targetExports)
    {
        uint8_t diskBuf[12] = {};
        uint8_t memBuf[12] = {};

        if (!CompareExportPrologue(ntdll, diskImage, expName, diskBuf, memBuf))
        {
            hooked = true;
            break;
        }

        if (memcmp(diskBuf, memBuf, 12) != 0)
        {
            hooked = true;
            break;
        }
    }

    Api::UnmapViewOfFile(diskImage);
    Api::CloseHandle(mapping);
    Api::CloseHandle(file);

    return hooked;
}
