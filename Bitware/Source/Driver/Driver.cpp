#include "Driver.h"
#include <Auth/skStr.h>

std::uint32_t Driver_t::Find_Process(const std::string& Process_Name)
{
    OBF_PROLOGUE;
    std::uint32_t Local_Process = 0;
    HANDLE Snapshot = Api::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Snapshot == INVALID_HANDLE_VALUE)
    {
        OBF_JUNK_BLOCK;
        return Local_Process;
    }

    PROCESSENTRY32 Process_Entry{};
    Process_Entry.dwSize = sizeof(PROCESSENTRY32);

    if (Api::Process32First(Snapshot, &Process_Entry))
    {
        do
        {
            OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
            if (!_stricmp(Process_Name.c_str(), Process_Entry.szExeFile))
            {
                Local_Process = Process_Entry.th32ProcessID;
                Process_ID = Local_Process;
                OBF_JUNK_BLOCK;
                break;
            }
        } while (Api::Process32Next(Snapshot, &Process_Entry));
    }

    Api::CloseHandle(Snapshot);
    return Local_Process;
}

std::uint64_t Driver_t::Find_Module(const std::string& Module_Name)
{
    OBF_PROLOGUE;
    std::uint64_t Module_Address = 0;

    if (!Process_Handle || Process_Handle == INVALID_HANDLE_VALUE)
    {
        OBF_JUNK_BLOCK;
        return Module_Address;
    }

    DWORD pid = Api::GetProcessId(Process_Handle);
    HANDLE Snapshot = Api::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);

    if (Snapshot == INVALID_HANDLE_VALUE)
    {
        OBF_JUNK_BLOCK;
        return Module_Address;
    }

    MODULEENTRY32 Module_Entry{};
    Module_Entry.dwSize = sizeof(MODULEENTRY32);

    if (Api::Module32First(Snapshot, &Module_Entry))
    {
        do
        {
            OBF_JUNK_DECLARE;
            if (!_stricmp(Module_Name.c_str(), Module_Entry.szModule))
            {
                Module_Address = reinterpret_cast<uint64_t>(Module_Entry.modBaseAddr);
                Base_Address = Module_Address;
                OBF_JUNK_BLOCK;
                break;
            }
        } while (Api::Module32Next(Snapshot, &Module_Entry));
    }

    Api::CloseHandle(Snapshot);
    return Module_Address;
}

bool Driver_t::Attach_Process(const std::string& Process_Name)
{
    OBF_PROLOGUE;
    HANDLE Process = Api::OpenProcess(PROCESS_ALL_ACCESS, false, Find_Process(Process_Name));

    if (!Process || Process == INVALID_HANDLE_VALUE)
    {
        OBF_JUNK_BLOCK;
        return false;
    }

    Process_Handle = Process;
    return true;
}

std::string Driver_t::Read_String(std::uint64_t Address)
{
    OBF_PROLOGUE;
    std::int32_t String_Length = Read<std::int32_t>(Address + 0x10);

    if (String_Length <= 0 || String_Length > 255)
    {
        OBF_JUNK_BLOCK;
        return std::string(skCrypt("Unknown"));
    }

    std::uint64_t String_Address = (String_Length >= 16) ? Read<std::uint64_t>(Address) : Address;

    std::vector<char> Buffer(String_Length + 1, 0);
    Driver_ReadVirtualMemory(Process_Handle, reinterpret_cast<void*>(String_Address), Buffer.data(), (ULONG)Buffer.size(), nullptr);

    return std::string(Buffer.data(), String_Length);
}

void Driver_t::Write_String(std::uint64_t Address, const std::string& Value)
{
    OBF_PROLOGUE;
    auto Str = Read<RbxString>(Address);

    if (Value.length() > Str.Capacity)
    {
        while (Value.length() > Str.Capacity)
            Str.Capacity = Str.Capacity * 2 + 1;

        Str.Data.Pointer = reinterpret_cast<std::uint64_t>(
            Api::VirtualAllocEx(
                Process_Handle,
                nullptr,
                Str.Capacity,
                MEM_RESERVE | MEM_COMMIT,
                PAGE_READWRITE
            )
        );

        if (!Str.Data.Pointer)
        {
            OBF_JUNK_BLOCK;
            return;
        }
    }

    Str.Length = Value.length();

    if (Str.Length > 15)
    {
        Write<RbxString>(Address, Str);
        Driver_WriteVirtualMemory(
            Process_Handle,
            reinterpret_cast<void*>(Str.Data.Pointer),
            (void*)Value.data(),
            (ULONG)Value.length(),
            nullptr
        );
    }
    else
    {
        Str.Capacity = 15;
        Write<RbxString>(Address, Str);
        Driver_WriteVirtualMemory(
            Process_Handle,
            reinterpret_cast<void*>(Address),
            (void*)Value.data(),
            (ULONG)Value.length(),
            nullptr
        );
    }
}

std::uint32_t Driver_t::Get_Process()
{
    return Process_ID;
}

std::uint64_t Driver_t::Get_Module()
{
    return Base_Address;
}

HANDLE Driver_t::Get_Handle()
{
    return Process_Handle;
}
