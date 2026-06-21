#pragma once
#include <windows.h>
#include <cstdint>

namespace EatParser {

    inline void* FindModuleInPEB(const wchar_t* module_name)
    {
        auto peb = __readgsqword(0x60);
        if (!peb) return nullptr;

        auto ldr = *reinterpret_cast<void**>(peb + 0x18);
        if (!ldr) return nullptr;

        auto head_flink = *reinterpret_cast<void**>(reinterpret_cast<uintptr_t>(ldr) + 0x20);
        if (!head_flink) return nullptr;

        auto entry = reinterpret_cast<uintptr_t>(head_flink) - 0x10;

        while (true)
        {
            auto dll_base = *reinterpret_cast<void**>(entry + 0x30);
            auto name_length = *reinterpret_cast<USHORT*>(entry + 0x58);
            auto name_buffer = *reinterpret_cast<const wchar_t**>(entry + 0x60);

            if (name_buffer && name_length > 0 && dll_base)
            {
                size_t name_chars = name_length / sizeof(wchar_t);
                size_t target_len = 0;
                while (module_name[target_len]) ++target_len;

                if (name_chars == target_len)
                {
                    bool match = true;
                    for (size_t i = 0; i < name_chars; ++i)
                    {
                        wchar_t a = name_buffer[i];
                        wchar_t b = module_name[i];
                        if (a >= L'A' && a <= L'Z') a += 32;
                        if (b >= L'A' && b <= L'Z') b += 32;
                        if (a != b) { match = false; break; }
                    }
                    if (match) return dll_base;
                }
            }

            auto next_flink = *reinterpret_cast<void**>(entry + 0x10);
            if (!next_flink || next_flink == head_flink)
                break;

            entry = reinterpret_cast<uintptr_t>(next_flink) - 0x10;
        }

        return nullptr;
    }

    static bool StringICmp(const char* a, const char* b)
    {
        while (*a && *b)
        {
            char ca = *a >= 'A' && *a <= 'Z' ? *a + 32 : *a;
            char cb = *b >= 'A' && *b <= 'Z' ? *b + 32 : *b;
            if (ca != cb) return false;
            ++a; ++b;
        }
        return *a == *b;
    }

    static bool HasDllExt(const char* name, size_t len)
    {
        if (len < 4) return false;
        return name[len - 4] == '.' &&
               (name[len - 3] == 'd' || name[len - 3] == 'D') &&
               (name[len - 2] == 'l' || name[len - 2] == 'L') &&
               (name[len - 1] == 'l' || name[len - 1] == 'L');
    }

    inline void* GetProcAddressEAT(void* module_base, const char* function_name)
    {
        if (!module_base || !function_name) return nullptr;

        auto base = reinterpret_cast<uintptr_t>(module_base);
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE) return nullptr;

        auto nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE) return nullptr;

        auto export_dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        if (!export_dir.VirtualAddress || !export_dir.Size) return nullptr;

        auto exports = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(base + export_dir.VirtualAddress);
        auto names = reinterpret_cast<uint32_t*>(base + exports->AddressOfNames);
        auto ordinals = reinterpret_cast<uint16_t*>(base + exports->AddressOfNameOrdinals);
        auto functions = reinterpret_cast<uint32_t*>(base + exports->AddressOfFunctions);

        for (uint32_t i = 0; i < exports->NumberOfNames; ++i)
        {
            auto name_ptr = reinterpret_cast<const char*>(base + names[i]);
            if (!StringICmp(name_ptr, function_name)) continue;

            auto ordinal = ordinals[i];
            auto function_rva = functions[ordinal];

            if (function_rva >= export_dir.VirtualAddress &&
                function_rva < export_dir.VirtualAddress + export_dir.Size)
            {
                auto forward_str = reinterpret_cast<const char*>(base + function_rva);
                auto dot = strchr(forward_str, '.');
                if (!dot || dot == forward_str) return nullptr;

                size_t dll_len = dot - forward_str;
                if (dll_len > 256) return nullptr;

                char dll_name[260]{};
                memcpy(dll_name, forward_str, dll_len);

                wchar_t wide_name[260]{};
                size_t wi = 0;
                for (; wi < dll_len; ++wi)
                    wide_name[wi] = static_cast<wchar_t>(dll_name[wi]);

                if (!HasDllExt(dll_name, dll_len))
                {
                    wide_name[wi++] = L'.';
                    wide_name[wi++] = L'd';
                    wide_name[wi++] = L'l';
                    wide_name[wi++] = L'l';
                }
                wide_name[wi] = 0;

                auto fwd_base = FindModuleInPEB(wide_name);
                if (!fwd_base) return nullptr;

                return GetProcAddressEAT(fwd_base, dot + 1);
            }

            return reinterpret_cast<void*>(base + function_rva);
        }

        return nullptr;
    }
}
