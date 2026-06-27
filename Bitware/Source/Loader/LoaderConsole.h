#pragma once
#include <windows.h>
#include <cstdio>
#include <Auth/skStr.h>

namespace LoaderConsole {

    inline HANDLE Handle() {
        static HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        return h;
    }

    inline void Init(const wchar_t* title) {
        AllocConsole();
        SetConsoleTitleW(title);
        FILE* _dummy;
        freopen_s(&_dummy, skCrypt("CONOUT$"), skCrypt("w"), stdout);
        SetConsoleOutputCP(CP_UTF8);
    }

    inline void Hide() {
        ShowWindow(GetConsoleWindow(), SW_HIDE);
    }

    inline void Shutdown() {
        fclose(stdout);
        FreeConsole();
    }

}
