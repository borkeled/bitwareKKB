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
    void BlockKey(DWORD vk, bool block);
    bool IsKeyBlocked(DWORD vk);
}
