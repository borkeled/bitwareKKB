#pragma once
#include <windows.h>
#include <cstdint>
#include <atomic>
#include <string>
#include <random>
#include <Auth/skStr.h>
#include <Infrastructure/ApiHiding.h>
#include <Infrastructure/Obfuscation.h>

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

    inline std::string GenerateWindowClass() {
        std::string names[] = {
            "MSCTF_Input",
            "MSTaskSwWClass",
            "Shell_TrayWnd",
            "SysListView32",
            "WorkerW",
            "Progman",
            "Windows.UI.Core.CoreWindow",
            "Button",
            "Edit",
            "ComboBox",
            "ScrollBar",
            "Static",
            "ListBox",
            "RichEdit20W",
            "SysTreeView32",
            "SysHeader32",
            "SysTabControl32",
            "SysAnimate32",
            "msctls_statusbar32",
            "msctls_progress32",
            "msctls_updown32",
            "msctls_hotkey32",
            "tooltips_class32",
            "SysIPAddress32",
            "SysMonthCal32",
            "SysDateTimePick32",
            "ReBarWindow32",
            "#32770",
            "MSTaskListWClass",
            "Shell_SecondaryTrayWnd",
            "DV2ControlHost",
            "SystemSettingsViewModelHost",
            "Windows.UI.Composition.DesktopWindowContentBridge",
            "ApplicationFrameWindow",
            "Windows.UI.Core.CoreFrameworkInputView",
            "Shell_DllWindowClass",
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
