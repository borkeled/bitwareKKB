#pragma once
#include <cstdint>
#include <windows.h>
#include <random>
#include <vector>
#include <mutex>
#include <Auth/skStr.h>

namespace SyscallObf {

    struct StubInfo {
        uint8_t* Address;
        size_t Size;
        uint64_t XorKey;
    };

    static std::mutex g_StubMutex;
    static std::vector<StubInfo> g_AllStubs;

    static void CleanupStubs() {
        std::lock_guard<std::mutex> lock(g_StubMutex);
        for (auto& stub : g_AllStubs) {
            if (stub.Address)
                VirtualFree(stub.Address, 0, MEM_RELEASE);
        }
        g_AllStubs.clear();
    }

    static std::mt19937_64& GetRng() {
        static std::mt19937_64 rng(std::random_device{}());
        return rng;
    }

    static void WriteSafeGarbage(uint8_t*& p, size_t min, size_t max) {
        if (max == 0) return;
        size_t len = min;
        if (max > min) {
            len = min + (GetRng()() % (max - min));
        }
        size_t written = 0;
        while (written < len) {
            size_t remaining = len - written;
            int choice = static_cast<int>(GetRng()() % 11);
            switch (choice) {
            case 0:
                *p++ = 0x90; written += 1;
                break;
            case 1:
                if (remaining >= 2) { *p++ = 0x66; *p++ = 0x90; written += 2; }
                else { *p++ = 0x90; written += 1; }
                break;
            case 2:
                if (remaining >= 3) { *p++ = 0x0F; *p++ = 0x1F; *p++ = 0x00; written += 3; }
                else { *p++ = 0x90; written += 1; }
                break;
            case 3:
                if (remaining >= 4) { *p++ = 0x0F; *p++ = 0x1F; *p++ = 0x40; *p++ = 0x00; written += 4; }
                else { *p++ = 0x90; written += 1; }
                break;
            case 4:
                if (remaining >= 5) { *p++ = 0x0F; *p++ = 0x1F; *p++ = 0x44; *p++ = 0x00; *p++ = 0x00; written += 5; }
                else { *p++ = 0x90; written += 1; }
                break;
            case 5:
                if (remaining >= 3) { *p++ = 0x4D; *p++ = 0x89; *p++ = 0xDB; written += 3; }
                else { *p++ = 0x90; written += 1; }
                break;
            case 6:
                if (remaining >= 3) { *p++ = 0x4D; *p++ = 0x89; *p++ = 0xF6; written += 3; }
                else { *p++ = 0x90; written += 1; }
                break;
            case 7:
                if (remaining >= 3) { *p++ = 0x4D; *p++ = 0x8D; *p++ = 0x1B; written += 3; }
                else { *p++ = 0x90; written += 1; }
                break;
            case 8:
                if (remaining >= 3) { *p++ = 0x4D; *p++ = 0x8D; *p++ = 0x3F; written += 3; }
                else { *p++ = 0x90; written += 1; }
                break;
            case 9:
                if (remaining >= 4) { *p++ = 0x41; *p++ = 0x54; *p++ = 0x41; *p++ = 0x5C; written += 4; }
                else { *p++ = 0x90; written += 1; }
                break;
            case 10:
                if (remaining >= 4) { *p++ = 0x41; *p++ = 0x57; *p++ = 0x41; *p++ = 0x5F; written += 4; }
                else { *p++ = 0x90; written += 1; }
                break;
            }
        }
    }

    static StubInfo GenerateSyscallStub(uint32_t ssn) {
        uint8_t buffer[256];
        uint8_t* p = buffer;

        WriteSafeGarbage(p, 0, 12);

        *p++ = 0x4C; *p++ = 0x8B; *p++ = 0xD1;

        WriteSafeGarbage(p, 5, 28);

        *p++ = 0xB8;
        *(uint32_t*)p = ssn;
        p += 4;

        WriteSafeGarbage(p, 3, 16);

        *p++ = 0x0F; *p++ = 0x05;

        WriteSafeGarbage(p, 0, 12);

        *p++ = 0xC3;

        size_t total = p - buffer;
        uint8_t* exec = (uint8_t*)VirtualAlloc(nullptr, ((total + 16) & ~15), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!exec) return { nullptr, 0, 0 };

        uint64_t xor_key = GetRng()();
        for (size_t i = 0; i < total; i++) {
            exec[i] = buffer[i] ^ ((uint8_t*)&xor_key)[i % 8];
        }

        StubInfo info = { exec, total, xor_key };
        {
            std::lock_guard<std::mutex> lock(g_StubMutex);
            g_AllStubs.push_back(info);
        }
        return info;
    }

    static void DecryptStub(StubInfo& stub) {
        if (!stub.Address || stub.XorKey == 0) return;
        DWORD old;
        VirtualProtect(stub.Address, stub.Size, PAGE_READWRITE, &old);
        for (size_t i = 0; i < stub.Size; i++) {
            stub.Address[i] ^= ((uint8_t*)&stub.XorKey)[i % 8];
        }
        VirtualProtect(stub.Address, stub.Size, PAGE_EXECUTE_READ, &old);
    }

    static StubInfo GenerateWriteStub(uint32_t offsX, uint32_t offsY) {
        uint8_t buffer[128];
        uint8_t* p = buffer;

        WriteSafeGarbage(p, 0, 10);

        *p++ = 0x48; *p++ = 0x8B; *p++ = 0xC2;

        WriteSafeGarbage(p, 3, 12);

        *p++ = 0x44; *p++ = 0x89; *p++ = 0x80;
        *(uint32_t*)p = offsX;
        p += 4;

        WriteSafeGarbage(p, 2, 8);

        *p++ = 0x44; *p++ = 0x89; *p++ = 0x88;
        *(uint32_t*)p = offsY;
        p += 4;

        WriteSafeGarbage(p, 0, 8);

        *p++ = 0xC3;

        size_t total = p - buffer;
        uint8_t* exec = (uint8_t*)VirtualAlloc(nullptr, ((total + 16) & ~15), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!exec) return { nullptr, 0, 0 };

        memcpy(exec, buffer, total);

        DWORD old;
        VirtualProtect(exec, ((total + 16) & ~15), PAGE_EXECUTE_READ, &old);

        StubInfo info = { exec, total, 0 };
        {
            std::lock_guard<std::mutex> lock(g_StubMutex);
            g_AllStubs.push_back(info);
        }
        return info;
    }

    static void EncryptStub(StubInfo& stub) {
        if (!stub.Address || stub.XorKey == 0) return;
        DWORD old;
        VirtualProtect(stub.Address, stub.Size, PAGE_READWRITE, &old);
        for (size_t i = 0; i < stub.Size; i++) {
            stub.Address[i] ^= ((uint8_t*)&stub.XorKey)[i % 8];
        }
        VirtualProtect(stub.Address, stub.Size, PAGE_READWRITE, &old);
    }
}
