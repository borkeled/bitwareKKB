#pragma once
#include <Windows.h>
#include <vector>

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

    static std::vector<HMODULE> baselineModules;
    static HANDLE hThread;
    static bool running;
};