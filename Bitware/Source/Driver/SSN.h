#pragma once
#include <cstdint>
#include <string>
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
        SyscallObf::CleanupStubs();

        auto ntdll = EatParser::FindModuleInPEB(skCrypt(L"ntdll.dll"));
        if (!ntdll) return false;

        struct StubEntry { const char* Name; void** Target; };
        static const std::string kNames[] = {
            std::string(skCrypt("NtReadVirtualMemory")),
            std::string(skCrypt("NtWriteVirtualMemory")),
            std::string(skCrypt("NtOpenProcess")),
            std::string(skCrypt("NtQuerySystemInformation")),
        };
        StubEntry stubs[] = {
            { kNames[0].c_str(), reinterpret_cast<void**>(&DriverReadVirtualMemory) },
            { kNames[1].c_str(), reinterpret_cast<void**>(&DriverWriteVirtualMemory) },
            { kNames[2].c_str(), reinterpret_cast<void**>(&DriverNtOpenProcess) },
            { kNames[3].c_str(), reinterpret_cast<void**>(&DriverNtQuerySystemInformation) },
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
