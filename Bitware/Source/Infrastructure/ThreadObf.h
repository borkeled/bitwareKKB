#pragma once
#include <windows.h>
#include <cstdint>
#include <atomic>
#include <string>
#include <random>
#include <Auth/skStr.h>
#include <Infrastructure/ApiHiding.h>
#include <Infrastructure/Obfuscation.h>

#ifndef ThreadHideFromDebugger
#define ThreadHideFromDebugger ((THREADINFOCLASS)0x11)
#endif

namespace ThreadObf {

    inline std::string RandomString(size_t len) {
        static const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        static std::mt19937_64 rng(std::random_device{}());
        static std::uniform_int_distribution<int> dist(0, sizeof(charset) - 2);
        std::string result(len, '\0');
        for (size_t i = 0; i < len; ++i) result[i] = charset[dist(rng)];
        return result;
    }

    inline std::wstring RandomStringWide(size_t len) {
        std::wstring result;
        result.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            result += L'A' + (rand() % 26);
        }
        return result;
    }

    inline DWORD WINAPI HiddenThreadProc(LPVOID) {
        return 0;
    }

    inline HANDLE CreateObfuscatedThread(LPTHREAD_START_ROUTINE Start, LPVOID Param) {
        auto ThreadFunc = [](LPVOID p) -> DWORD {
            OBF_PROLOGUE;
            auto real_fn = reinterpret_cast<LPTHREAD_START_ROUTINE>(p);
            return real_fn(nullptr);
        };

        DWORD tid;
        HANDLE hThread = Api::CreateThread(nullptr, 0, ThreadFunc, Start, 0, &tid);
        if (hThread) {
            HMODULE ntdll = reinterpret_cast<HMODULE>(Api::GetModuleHandleA(skCrypt("ntdll.dll")));
            if (ntdll) {
                typedef NTSTATUS(NTAPI* NtSetInformationThread_t)(HANDLE, THREADINFOCLASS, PVOID, ULONG);
                auto NtSetInformationThread = reinterpret_cast<NtSetInformationThread_t>(
                    Api::GetProcAddress(reinterpret_cast<HMODULE>(ntdll), skCrypt("NtSetInformationThread")));
                if (NtSetInformationThread) {
                    ULONG hide = 1;
                    NtSetInformationThread(hThread, ThreadHideFromDebugger, &hide, sizeof(hide));
                }
            }
        }
        return hThread;
    }

    inline std::string GenerateWindowClass() {
        std::string names[] = {
            "MSCTF_Input",
            "MSTaskSwWClass",
            "Shell_TrayWnd",
            "SysListView32",
            "WorkerW",
            "Progman",
            "Windows.UI.Core.CoreWindow",
        };
        return names[rand() % (sizeof(names) / sizeof(names[0]))];
    }

    inline void RandomizeThreadNames() {
        std::thread t([]() {
            auto thread = Api::GetCurrentThread();
            typedef HRESULT(WINAPI* SetThreadDescription_t)(HANDLE, PCWSTR);
            auto kernel32 = reinterpret_cast<HMODULE>(Api::GetModuleHandleA(skCrypt("kernel32.dll")));
            if (kernel32) {
                auto SetThreadDescription = reinterpret_cast<SetThreadDescription_t>(
                    Api::GetProcAddress(kernel32, skCrypt("SetThreadDescription")));
                if (SetThreadDescription) {
                    auto name = RandomStringWide(8 + rand() % 12);
                    SetThreadDescription(thread, name.c_str());
                }
            }
        });
        t.detach();
    }
}
