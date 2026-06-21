#include "InputHook.h"
#include <Globals.hxx>
#include <Core/Graphics/Graphics.h>
#include <Infrastructure/ApiHiding.h>
#include <Infrastructure/Obfuscation.h>
#include <ImGui/addons/imgui_addons.h>

namespace InputHook
{
    static HHOOK g_hKeyboardHook = nullptr;
    static volatile bool g_KeyStates[256] = {};
    static volatile bool g_MenuKeyPressed = false;

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

    static bool IsVKBound(DWORD vk)
    {
        int binds[] = {
            ImGuiKeyToVK(SettingsStore::Settings_Menu_Keybind),
            ImGuiKeyToVK(SettingsStore::Aimbot_Key),
            ImGuiKeyToVK(SettingsStore::Aimbot_FovToggleKey),
            ImGuiKeyToVK(SettingsStore::Silent_Key),
            ImGuiKeyToVK(SettingsStore::Visuals_ToggleKey),
            ImGuiKeyToVK(SettingsStore::Triggerbot_Key),
            ImGuiKeyToVK(SettingsStore::Misc_Speed_Key),
            ImGuiKeyToVK(SettingsStore::Misc_Jump_Key),
        };
        for (int i = 0; i < 8; i++)
            if (binds[i] != 0 && vk == static_cast<DWORD>(binds[i]))
                return true;
        return false;
    }

    static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;

        if (nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN || wParam == WM_KEYUP || wParam == WM_SYSKEYUP))
        {
            KBDLLHOOKSTRUCT* pKbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
            DWORD vk = pKbd->vkCode;

            if (vk < 256)
            {
                if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
                    g_KeyStates[vk] = true;
                else
                    g_KeyStates[vk] = false;
            }

            bool menuOpen = false;

            if (Graphic && Graphic->Running)
                menuOpen = true;

            if (vk == static_cast<DWORD>(ImGuiKeyToVK(SettingsStore::Settings_Menu_Keybind)))
            {
                if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
                    g_MenuKeyPressed = true;
            }

            if (!menuOpen && IsVKBound(vk))
            {
                OBF_JUNK_BLOCK;
                return 1;
            }
        }

        OBF_JUNK_BLOCK;
        return Api::CallNextHookEx(nullptr, nCode, wParam, lParam);
    }

    void PollMouseKeys()
    {
        bool lButton = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        bool rButton = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
        bool mButton = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
        bool x1 = (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;
        bool x2 = (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0;

        g_KeyStates[VK_LBUTTON] = lButton;
        g_KeyStates[VK_RBUTTON] = rButton;
        g_KeyStates[VK_MBUTTON] = mButton;
        g_KeyStates[VK_XBUTTON1] = x1;
        g_KeyStates[VK_XBUTTON2] = x2;

        // Rising edge detection for menu key when bound to a mouse button
        DWORD menuVk = static_cast<DWORD>(ImGuiKeyToVK(SettingsStore::Settings_Menu_Keybind));
        if (menuVk >= VK_LBUTTON && menuVk <= VK_XBUTTON2)
        {
            static bool prevMenuMouse = false;
            bool held = g_KeyStates[menuVk];
            if (held && !prevMenuMouse)
                g_MenuKeyPressed = true;
            prevMenuMouse = held;
        }
    }

    bool Install()
    {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;

        g_hKeyboardHook = Api::SetWindowsHookExA(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandleA(nullptr), 0);
        if (!g_hKeyboardHook)
        {
            OBF_JUNK_BLOCK;
            return false;
        }

        OBF_JUNK_BLOCK;
        return true;
    }

    void Remove()
    {
        OBF_PROLOGUE;
        OBF_JUNK_DECLARE;

        if (g_hKeyboardHook)
        {
            Api::UnhookWindowsHookEx(g_hKeyboardHook);
            g_hKeyboardHook = nullptr;
        }

        OBF_JUNK_BLOCK;
    }
}
