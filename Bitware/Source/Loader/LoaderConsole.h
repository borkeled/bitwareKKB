#pragma once
#include <windows.h>
#include <cstdio>
#include <Auth/skStr.h>

namespace LoaderConsole {

    enum Color {
        White = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
        Green = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        Yellow = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
        Red = FOREGROUND_RED | FOREGROUND_INTENSITY,
        Cyan = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
        Gray = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
    };

    inline HANDLE Handle() {
        static HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        return h;
    }

    inline void SetColor(Color c) {
        SetConsoleTextAttribute(Handle(), c);
    }

    inline void Init(const wchar_t* title) {
        AllocConsole();
        SetConsoleTitleW(title);
        FILE* _dummy;
        freopen_s(&_dummy, skCrypt("CONOUT$"), skCrypt("w"), stdout);
        SetConsoleOutputCP(CP_UTF8);
        SetColor(White);
    }

    inline void Print(Color c, const char* text) {
        SetColor(c);
        printf(skCrypt("%s"), text);
        SetColor(White);
    }

    inline void PrintLine(Color c, const char* text) {
        Print(c, text);
        printf(skCrypt("\n"));
    }

    inline void PrintLine() {
        printf(skCrypt("\n"));
    }

    inline void PrintStatus(const char* label, const char* message) {
        SetColor(Yellow);
        printf(skCrypt("["));
        SetColor(Cyan);
        printf(skCrypt("%s"), label);
        SetColor(Yellow);
        printf(skCrypt("]"));
        SetColor(White);
        printf(skCrypt(" %s\n"), message);
        SetColor(White);
    }

    inline void PrintSuccess(const char* label) {
        SetColor(Green);
        printf(skCrypt("  ✓ %s\n"), label);
        SetColor(White);
    }

    inline void Hide() {
        ShowWindow(GetConsoleWindow(), SW_HIDE);
    }

    inline void Shutdown() {
        fclose(stdout);
        FreeConsole();
    }

    inline void SetCursorVisible(bool visible) {
        CONSOLE_CURSOR_INFO ci;
        GetConsoleCursorInfo(Handle(), &ci);
        ci.bVisible = visible;
        SetConsoleCursorInfo(Handle(), &ci);
    }

    inline void ClearLine() {
        printf(skCrypt("\r%*s\r"), 120, skCrypt(""));
        fflush(stdout);
    }

    inline char SpinnerFrame(int& frame) {
        static const char frames[] = { '|', '/', '-', '\\' };
        return frames[frame++ & 3];
    }

    inline void DrawProgressBar(int percent) {
        SetColor(Cyan);
        printf(skCrypt("\r  ["));
        int pos = percent / 5;
        for (int i = 0; i < 20; i++) {
            if (i < pos) printf(skCrypt("#"));
            else printf(skCrypt("-"));
        }
        SetColor(White);
        printf(skCrypt("] %3d%%"), percent);
        fflush(stdout);
    }

}
