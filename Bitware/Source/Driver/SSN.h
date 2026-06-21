#pragma once
#include <cstdint>
#include <windows.h>
#include <Infrastructure/EatParser.h>
#include <Infrastructure/ApiHiding.h>
#include <Infrastructure/SyscallObf.h>
#include <Driver/Driver.h>

namespace SSN {

    inline int ExtractFromStub(void* stub_addr)
    {
        auto bytes = static_cast<uint8_t*>(stub_addr);
        for (int i = 0; i < 32; i++)
        {
            if (bytes[i] == 0xB8)
                return *reinterpret_cast<int*>(&bytes[i + 1]);
        }
        return -1;
    }

    inline bool Resolve()
    {
        auto ntdll = EatParser::FindModuleInPEB(L"ntdll.dll");
        if (!ntdll) return false;

        struct StubEntry { const char* Name; void** Target; };
        StubEntry stubs[] = {
            { "NtReadVirtualMemory", reinterpret_cast<void**>(&DriverReadVirtualMemory) },
            { "NtWriteVirtualMemory", reinterpret_cast<void**>(&DriverWriteVirtualMemory) },
            { "NtOpenProcess", reinterpret_cast<void**>(&DriverNtOpenProcess) },
            { "NtQuerySystemInformation", reinterpret_cast<void**>(&DriverNtQuerySystemInformation) },
        };

        for (auto& entry : stubs) {
            void* stub = EatParser::GetProcAddressEAT(ntdll, entry.Name);
            if (!stub) continue;

            int ssn = ExtractFromStub(stub);
            if (ssn <= 0) continue;

            auto info = SyscallObf::GenerateSyscallStub(static_cast<uint32_t>(ssn));
            if (!info.Address) continue;

            SyscallObf::DecryptStub(info);
            *entry.Target = info.Address;
        }

        auto writeInfo = SyscallObf::GenerateWriteStub(0xEC, 0xF0);
        if (writeInfo.Address) {
            DriverWriteMousePosition = reinterpret_cast<WriteMousePosFn>(writeInfo.Address);
        }

        return DriverReadVirtualMemory != nullptr && DriverWriteVirtualMemory != nullptr;
    }
}
