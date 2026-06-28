#pragma once
#include <Windows.h>

class AntiInjection
{
public:
    static void Start();
    static void Stop();

private:
    static DWORD __stdcall MonitorThread(void* lpParam);

    static bool DetectDebugger();
    static bool DetectRemoteDebugger();
    static bool DetectNewModules();
    static bool DetectManualMapRegions();
    static bool DetectThreadStartAnomalies();
    static bool DetectNtdllHooks();

    static HMODULE baselineModules[256];
    static int baselineModuleCount;
    static HANDLE hThread;
    static bool running;
    static PVOID dllNotifyCookie;
};