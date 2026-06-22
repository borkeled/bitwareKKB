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
        static const std::string charset(skCrypt("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"));
        static std::mt19937_64 rng(std::random_device{}());
        static std::uniform_int_distribution<int> dist(0, (int)charset.size() - 2);
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
        static const std::string names[] = {
            std::string(skCrypt("MSCTF_Input")),
            std::string(skCrypt("MSTaskSwWClass")),
            std::string(skCrypt("Shell_TrayWnd")),
            std::string(skCrypt("SysListView32")),
            std::string(skCrypt("WorkerW")),
            std::string(skCrypt("Progman")),
            std::string(skCrypt("Windows.UI.Core.CoreWindow")),
            std::string(skCrypt("Button")),
            std::string(skCrypt("Edit")),
            std::string(skCrypt("ComboBox")),
            std::string(skCrypt("ScrollBar")),
            std::string(skCrypt("Static")),
            std::string(skCrypt("ListBox")),
            std::string(skCrypt("RichEdit20W")),
            std::string(skCrypt("SysTreeView32")),
            std::string(skCrypt("SysHeader32")),
            std::string(skCrypt("SysTabControl32")),
            std::string(skCrypt("SysAnimate32")),
            std::string(skCrypt("msctls_statusbar32")),
            std::string(skCrypt("msctls_progress32")),
            std::string(skCrypt("msctls_updown32")),
            std::string(skCrypt("msctls_hotkey32")),
            std::string(skCrypt("tooltips_class32")),
            std::string(skCrypt("SysIPAddress32")),
            std::string(skCrypt("SysMonthCal32")),
            std::string(skCrypt("SysDateTimePick32")),
            std::string(skCrypt("ReBarWindow32")),
            std::string(skCrypt("#32770")),
            std::string(skCrypt("MSTaskListWClass")),
            std::string(skCrypt("Shell_SecondaryTrayWnd")),
            std::string(skCrypt("DV2ControlHost")),
            std::string(skCrypt("SystemSettingsViewModelHost")),
            std::string(skCrypt("Windows.UI.Composition.DesktopWindowContentBridge")),
            std::string(skCrypt("ApplicationFrameWindow")),
            std::string(skCrypt("Windows.UI.Core.CoreFrameworkInputView")),
            std::string(skCrypt("Shell_DllWindowClass")),
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
