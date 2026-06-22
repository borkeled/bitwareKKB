#include "InputHook.h"
#include <Globals.hxx>
#include <Infrastructure/Obfuscation.h>
#include <ImGui/addons/imgui_addons.h>

namespace InputHook
{
    static volatile bool g_KeyStates[256] = {};
    static volatile bool g_MenuKeyPressed = false;
    static bool g_BlockedKeys[256] = {};
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

    void BlockKey(DWORD vk, bool block)
    {
        if (vk >= 256) return;
        g_BlockedKeys[vk] = block;
    }

    bool IsKeyBlocked(DWORD vk)
    {
        if (vk >= 256) return false;
        return g_BlockedKeys[vk];
    }

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode != HC_ACTION)
            return CallNextHookEx(NULL, nCode, wParam, lParam);

        KBDLLHOOKSTRUCT* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        DWORD vk = kb->vkCode;

        if (vk >= 256)
            return CallNextHookEx(NULL, nCode, wParam, lParam);

        DWORD menuVk = static_cast<DWORD>(ImGuiKeyToVK(SettingsStore::Settings_Menu_Keybind));
        bool isMenuKey = (menuVk < 256 && vk == menuVk);

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            if (isMenuKey)
            {
                g_MenuKeyPressed = true;
                return 1;
            }

            if (!g_MenuOpen && g_BlockedKeys[vk])
            {
                g_KeyStates[vk] = true;
                return 1;
            }
        }

        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
        {
            if (isMenuKey)
                return 1;

            if (!g_MenuOpen && g_BlockedKeys[vk])
            {
                g_KeyStates[vk] = false;
                return 1;
            }
        }

        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    void PollKeys()
    {
        OBF_PROLOGUE;

        for (int i = 0; i < 256; i++)
        {
            if (g_BlockedKeys[i])
                continue;

            g_KeyStates[i] = (GetAsyncKeyState(i) & 0x8000) != 0;
        }

        DWORD menuVk = static_cast<DWORD>(ImGuiKeyToVK(SettingsStore::Settings_Menu_Keybind));
        if (menuVk < 256 && !g_BlockedKeys[menuVk])
        {
            static bool prevMenu = false;
            bool held = g_KeyStates[menuVk];
            if (held && !prevMenu)
                g_MenuKeyPressed = true;
            prevMenu = held;
        }
    }

    bool Install()
    {
        g_KeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
        return g_KeyboardHook != NULL;
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
