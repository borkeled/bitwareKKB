#pragma once
#include <windows.h>

namespace InputHook
{
    bool Install();
    void Remove();
    void PollMouseKeys();
    bool IsKeyDown(DWORD vk);
    bool ConsumeMenuKeyPress();
}
