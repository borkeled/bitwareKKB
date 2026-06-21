#include "AntiInjection.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <winternl.h>
#include <Auth/skStr.h>
#include <Infrastructure/ApiHiding.h>


std::vector<HMODULE> AntiInjection::baselineModules;
HANDLE AntiInjection::hThread = nullptr;
bool AntiInjection::running = false;

static std::vector<HMODULE> GetModules()
{
    std::vector<HMODULE> mods;
    HMODULE buffer[1024];
    DWORD needed = 0;
    if (Api::EnumProcessModules(Api::GetCurrentProcess(), buffer, sizeof(buffer), &needed))
    {
        size_t count = needed / sizeof(HMODULE);
        if (count > 1024) count = 1024; // Limit to maximum buffer size to prevent out-of-bounds reads
        for (size_t i = 0; i < count; ++i) mods.push_back(buffer[i]);
    }
    return mods;
}

void AntiInjection::Start()
{
    if (running) return;
    baselineModules = GetModules();
    running = true;
    hThread = Api::CreateThread(nullptr, 0, MonitorThread, nullptr, 0, nullptr);
}

void AntiInjection::Stop()
{
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
    while (running)
    {
        if (DetectDebugger() || DetectRemoteDebugger() || DetectNewModules() || DetectManualMapRegions() || DetectThreadStartAnomalies() || DetectNtdllHooks())
        {
            Api::TerminateProcess(Api::GetCurrentProcess(), 0);
        }
        Api::Sleep(500);
    }
    return 0;
}

bool AntiInjection::DetectDebugger() { return Api::IsDebuggerPresent(); }
bool AntiInjection::DetectRemoteDebugger() { BOOL present = FALSE; Api::CheckRemoteDebuggerPresent(Api::GetCurrentProcess(), &present); return present; }
bool AntiInjection::DetectNewModules()
{
    auto now = GetModules();
    for (HMODULE m : now)
    {
        bool known = false;
        for (HMODULE b : baselineModules) { if (m == b) { known = true; break; } }
        if (!known)
        {
            wchar_t path[MAX_PATH] = {};
            if (Api::GetModuleFileNameExW(Api::GetCurrentProcess(), m, path, MAX_PATH))
            {
                for (int i = 0; path[i]; ++i)
                {
                    if (path[i] >= L'A' && path[i] <= L'Z')
                    {
                        path[i] += 32;
                    }
                }

                if (wcsstr(path, skCrypt(L"\\windows\\")) || wcsstr(path, skCrypt(L"\\program files\\windows")))
                    continue;
            }
            return true;
        }
    }
    return false;
}

bool AntiInjection::DetectManualMapRegions()
{
    std::vector<uintptr_t> knownBases;
    HMODULE buffer[1024];
    DWORD needed;
    if (Api::EnumProcessModules(Api::GetCurrentProcess(), buffer, sizeof(buffer), &needed))
    {
        size_t count = needed / sizeof(HMODULE);
        for (size_t i = 0; i < count; i++)
            knownBases.push_back((uintptr_t)buffer[i]);
    }

    SYSTEM_INFO si;
    Api::GetSystemInfo(&si);
    uintptr_t addr = (uintptr_t)si.lpMinimumApplicationAddress;

    while (addr < (uintptr_t)si.lpMaximumApplicationAddress)
    {
        MEMORY_BASIC_INFORMATION mbi;
        if (Api::VirtualQuery((const void*)addr, &mbi, sizeof(mbi)) == 0)
        {
            addr += 0x10000;
            continue;
        }

        if (mbi.State == MEM_COMMIT && mbi.Type == MEM_IMAGE)
        {
            bool found = false;
            for (auto base : knownBases)
            {
                if ((uintptr_t)mbi.AllocationBase == base)
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
            HMODULE buffer[1024];
            DWORD needed;
            if (Api::EnumProcessModules(Api::GetCurrentProcess(), buffer, sizeof(buffer), &needed))
            {
                size_t count = needed / sizeof(HMODULE);
                for (size_t i = 0; i < count; i++)
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

    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)diskImage;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((uintptr_t)diskImage + dos->e_lfanew);
    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(nt);

    bool hooked = false;
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++)
    {
        if (memcmp(section[i].Name, ".text", 5) == 0)
        {
            void* diskText = (void*)((uintptr_t)diskImage + section[i].PointerToRawData);
            void* memText = (void*)((uintptr_t)ntdll + section[i].VirtualAddress);
            SIZE_T size = section[i].SizeOfRawData;
            if (size > 2048) size = 2048;

            if (memcmp(diskText, memText, size) != 0)
                hooked = true;
            break;
        }
    }

    Api::UnmapViewOfFile(diskImage);
    Api::CloseHandle(mapping);
    Api::CloseHandle(file);

    return hooked;
}
