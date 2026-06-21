#include "AntiInjection.h"
#include <TlHelp32.h>
#include <Psapi.h>
#include <winternl.h>
#include <Auth/skStr.h>
#include <Infrastructure/ApiHiding.h>


typedef NTSTATUS(NTAPI* NtQueryInformationThread_t)(HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);
static NtQueryInformationThread_t pNtQueryInformationThread = nullptr;

static void InitNtQueryInformationThread()
{
    HMODULE ntdll = GetModuleHandleW(skCrypt(L"ntdll.dll"));
    if (!ntdll) return;

    pNtQueryInformationThread = (NtQueryInformationThread_t)Api::GetProcAddress(ntdll, skCrypt("NtQueryInformationThread"));
}

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
    InitNtQueryInformationThread();

    if (running) return;
    baselineModules = GetModules();
    running = true;
    hThread = CreateThread(nullptr, 0, MonitorThread, nullptr, 0, nullptr);
}

void AntiInjection::Stop()
{
    running = false;
    if (hThread)
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
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
    // too aggressuve
    return false;
}

bool AntiInjection::DetectThreadStartAnomalies()
{
    //disablked
    return false;
}

bool AntiInjection::DetectNtdllHooks()
{
    //disabled
    return false;
}
