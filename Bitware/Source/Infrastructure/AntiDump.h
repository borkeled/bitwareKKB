#pragma once
#include <windows.h>
#include <cstdint>
#include <atomic>
#include <cstdio>
#include <Auth/skStr.h>
#include <Infrastructure/ApiHiding.h>
#include <Infrastructure/Logger.h>
#include <Miscellaneous/Protection/External/oxorany_include.h>

namespace AntiDump {

    static std::atomic<bool> ProtectionActive{ false };
    static PVOID VexHandle = nullptr;
    static DWORD OriginalProtect = PAGE_EXECUTE_READ;
    static std::atomic<std::uint64_t> SectionBase{ 0 };
    static std::atomic<SIZE_T> SectionSize{ 0 };

    #pragma section(".vcdata", read, execute)
    __declspec(code_seg(".vcdata"))
    __declspec(noinline)
    static LONG WINAPI OnException(PEXCEPTION_POINTERS ExceptionInfo) {
        auto code = ExceptionInfo->ExceptionRecord->ExceptionCode;
        auto addr = static_cast<std::uint64_t>(ExceptionInfo->ExceptionRecord->ExceptionInformation[1]);
        auto base = SectionBase.load();
        auto size = SectionSize.load();

        if (code == STATUS_ACCESS_VIOLATION && addr >= base && addr < base + size) {
            DWORD old;
            Api::VirtualProtect(reinterpret_cast<LPVOID>(base), size, OriginalProtect, &old);
            ExceptionInfo->ContextRecord->EFlags |= 0x100;
            return EXCEPTION_CONTINUE_EXECUTION;
        }

        if (code == STATUS_SINGLE_STEP) {
            DWORD old;
            Api::VirtualProtect(reinterpret_cast<LPVOID>(base), size, PAGE_NOACCESS, &old);
            return EXCEPTION_CONTINUE_EXECUTION;
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }

    static DWORD MapSectionCharacteristics(DWORD characteristics) {
        if (characteristics & IMAGE_SCN_MEM_EXECUTE) {
            if (characteristics & IMAGE_SCN_MEM_WRITE)
                return PAGE_EXECUTE_READWRITE;
            return PAGE_EXECUTE_READ;
        }
        if (characteristics & IMAGE_SCN_MEM_WRITE)
            return PAGE_READWRITE;
        if (characteristics & IMAGE_SCN_MEM_READ)
            return PAGE_READONLY;
        return PAGE_NOACCESS;
    }

    static std::uint64_t GetCurrentModuleBase() {
        HMODULE hModule = nullptr;
        GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&GetCurrentModuleBase),
            &hModule
        );
        return reinterpret_cast<std::uint64_t>(hModule);
    }

    static void GetTextSection(std::uint64_t& base, SIZE_T& size, DWORD& protect) {
        auto image_base = GetCurrentModuleBase();
        if (!image_base) return;
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(image_base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return;
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(image_base + dos->e_lfanew);
        auto sect = IMAGE_FIRST_SECTION(nt);
        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
            char name[9] = {};
            memcpy(name, sect[i].Name, 8);
            if (memcmp(name, ".text", 5) == 0 || memcmp(name, "CODE", 4) == 0) {
                base = image_base + sect[i].VirtualAddress;
                size = sect[i].Misc.VirtualSize;
                protect = MapSectionCharacteristics(sect[i].Characteristics);
                return;
            }
        }
    }

    inline bool Enable() {
        if (ProtectionActive.load()) return true;

        std::uint64_t sb = 0;
        SIZE_T sz = 0;
        GetTextSection(sb, sz, OriginalProtect);
        if (!sb || !sz) return false;
        SectionBase.store(sb);
        SectionSize.store(sz);

        VexHandle = AddVectoredExceptionHandler(1, OnException);
        if (!VexHandle) return false;

        ProtectionActive.store(true);

        DWORD old;
        BOOL vpResult = Api::VirtualProtect(reinterpret_cast<LPVOID>(sb), sz, PAGE_NOACCESS, &old);
        if (!vpResult) {
            RemoveVectoredExceptionHandler(VexHandle);
            VexHandle = nullptr;
            ProtectionActive.store(false);
            return false;
        }

        return true;
    }

    inline void Disable() {
        if (!ProtectionActive.load()) return;

        if (VexHandle) {
            RemoveVectoredExceptionHandler(VexHandle);
            VexHandle = nullptr;
        }

        auto base = SectionBase.load();
        auto size = SectionSize.load();
        if (base && size) {
            DWORD old;
            Api::VirtualProtect(reinterpret_cast<LPVOID>(base), size, OriginalProtect, &old);
        }

        ProtectionActive.store(false);
    }

    inline bool IsProtected() {
        return ProtectionActive.load();
    }

    inline void Toggle() {
        if (ProtectionActive.load()) {
            Disable();
        } else {
            Enable();
        }
    }
}
