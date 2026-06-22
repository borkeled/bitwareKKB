#pragma once
#include <windows.h>
#include <cstdio>
#include <Auth/skStr.h>

namespace Logger {

    inline bool Enabled = false;

    inline HANDLE GetHandle() {
        static HANDLE h = CreateFileA(skCrypt("bitware_debug.log"), GENERIC_WRITE, FILE_SHARE_READ,
            nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        return h;
    }

    inline void Log(const char* msg) {
        if (!Enabled) return;
        HANDLE h = GetHandle();
        if (h && h != INVALID_HANDLE_VALUE) {
            DWORD written;
            WriteFile(h, msg, (DWORD)strlen(msg), &written, nullptr);
            WriteFile(h, skCrypt("\r\n"), 2, &written, nullptr);
        }
    }

    inline void LogHex(const char* prefix, uintptr_t val) {
        if (!Enabled) return;
        char buf[128];
        int n = snprintf(buf, sizeof(buf), "%s 0x%llX", prefix, (unsigned long long)val);
        HANDLE h = GetHandle();
        if (h && h != INVALID_HANDLE_VALUE) {
            DWORD written;
            WriteFile(h, buf, (DWORD)n, &written, nullptr);
            WriteFile(h, skCrypt("\r\n"), 2, &written, nullptr);
        }
    }

    inline void Flush() {
        if (!Enabled) return;
        HANDLE h = GetHandle();
        if (h && h != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(h);
        }
    }
}

