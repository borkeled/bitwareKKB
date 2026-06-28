#include "InputHook.h"
#include <Globals.hxx>
#include <Infrastructure/Obfuscation.h>
#include <ImGui/addons/imgui_addons.h>

namespace InputHook
{
    static volatile bool g_KeyStates[256] = {};
    static volatile bool g_MenuKeyPressed = false;
    static bool g_BoundVk[256] = {};
    static bool g_MenuOpen = false;
    static HHOOK g_KeyboardHook = NULL;

    bool IsKeyDown(DWORD vk)
    {
        if (vk >= 256) return false;
        return g_KeyStates[vk];
    }

    bool ConsumeMenuKeyPress()
    {
        bool v = g_MenuKeyPressed;
        g_MenuKeyPressed = false;
        return v;
    }

    void SetMenuOpen(bool open)
    {
        g_MenuOpen = open;
    }

    bool IsMenuOpen()
    {
        return g_MenuOpen;
    }

    bool IsVkBound(DWORD vk)
    {
        if (vk >= 256) return false;
        return g_BoundVk[vk];
    }

    void PollKeys()
    {
        OBF_PROLOGUE;

        for (int i = 0; i < 256; i++)
        {
            g_KeyStates[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
        }

        DWORD menuVk = static_cast<DWORD>(ImGuiKeyToVK(SettingsStore::Settings_Menu_Keybind));
        if (menuVk < 256)
        {
            static bool prevMenu = false;
            bool held = g_KeyStates[menuVk];
            if (held && !prevMenu)
                g_MenuKeyPressed = true;
            prevMenu = held;
        }

        for (int i = 0; i < 256; i++)
            g_BoundVk[i] = false;

        auto bind = [](ImGuiKey key) {
            if (key == ImGuiKey_None) return;
            DWORD vk = static_cast<DWORD>(ImGuiKeyToVK(key));
            if (vk > 0 && vk < 256) g_BoundVk[vk] = true;
        };

        bind(SettingsStore::Aimbot_Key);
        bind(SettingsStore::Triggerbot_Key);
        bind(SettingsStore::Visuals_ToggleKey);
        bind(SettingsStore::Aimbot_FovToggleKey);
        bind(SettingsStore::Misc_Speed_Key);
        bind(SettingsStore::Misc_Jump_Key);
        bind(SettingsStore::Silent_Key);
        bind(SettingsStore::Silent_FovToggleKey);
    }

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode >= 0)
        {
            if (!g_MenuOpen)
            {
                KBDLLHOOKSTRUCT* p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
                if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
                {
                    if (p->vkCode < 256 && g_BoundVk[p->vkCode])
                    {
                        return 1;
                    }
                }
            }
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    bool Install()
    {
        g_KeyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandleA(NULL), 0);
        return true;
    }

    void Remove()
    {
        if (g_KeyboardHook)
        {
            UnhookWindowsHookEx(g_KeyboardHook);
            g_KeyboardHook = NULL;
        }
    }
}
