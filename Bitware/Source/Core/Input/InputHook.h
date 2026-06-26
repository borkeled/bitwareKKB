#pragma once
#include <windows.h>

namespace InputHook
{
    bool Install();
    void Remove();
    void PollKeys();
    bool IsKeyDown(DWORD vk);
    bool ConsumeMenuKeyPress();
    void SetMenuOpen(bool open);
    bool IsMenuOpen();
    bool IsVkBound(DWORD vk);
}
