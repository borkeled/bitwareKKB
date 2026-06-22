#include "InputHook.h"
#include <Globals.hxx>
#include <Infrastructure/Obfuscation.h>
#include <ImGui/addons/imgui_addons.h>

namespace InputHook
{
    static volatile bool g_KeyStates[256] = {};
    static volatile bool g_MenuKeyPressed = false;
    static bool g_BlockedKeys[256] = {};

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

    void SetMenuOpen(bool)
    {
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
    }

    bool Install()
    {
        return true;
    }

    void Remove()
    {
    }
}
