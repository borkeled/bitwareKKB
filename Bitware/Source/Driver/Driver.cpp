#include "Driver.h"
#include <Auth/skStr.h>
#include <psapi.h>

UsermodeMemory::~UsermodeMemory()
{
    Detach_Process();
}

bool UsermodeMemory::ReadRaw(std::uint64_t Address, void* Buffer, size_t Size)
{
    OBF_PROLOGUE;
    if (!DriverReadVirtualMemory || !m_ProcessHandle)
        return false;

    DriverReadVirtualMemory(m_ProcessHandle, reinterpret_cast<void*>(Address),
        Buffer, static_cast<ULONG>(Size), nullptr);
    return true;
}

bool UsermodeMemory::WriteRaw(std::uint64_t Address, const void* Buffer, size_t Size)
{
    OBF_PROLOGUE;
    if (!DriverWriteVirtualMemory || !m_ProcessHandle)
        return false;

    DriverWriteVirtualMemory(m_ProcessHandle, reinterpret_cast<void*>(Address),
        const_cast<void*>(Buffer), static_cast<ULONG>(Size), nullptr);
    return true;
}

std::uint32_t UsermodeMemory::Find_Process(const std::string& Process_Name)
{
    OBF_PROLOGUE;
    std::uint32_t Local_Process = 0;

    if (!DriverNtQuerySystemInformation) return Local_Process;

    ULONG buffer_size = 0x10000;
    std::vector<uint8_t> buffer(buffer_size);

    NTSTATUS status = DriverNtQuerySystemInformation(5, buffer.data(), buffer_size, &buffer_size);
    if (status == 0xC0000004) {
        buffer.resize(buffer_size);
        status = DriverNtQuerySystemInformation(5, buffer.data(), buffer_size, &buffer_size);
    }
    if (status < 0) return Local_Process;

    auto entry = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(buffer.data());
    while (true) {
        if (entry->ImageName.Buffer && entry->ImageName.Length > 0) {
            int len = WideCharToMultiByte(CP_UTF8, 0, entry->ImageName.Buffer,
                entry->ImageName.Length / sizeof(wchar_t), nullptr, 0, nullptr, nullptr);
            if (len > 0) {
                std::string name(len, '\0');
                WideCharToMultiByte(CP_UTF8, 0, entry->ImageName.Buffer,
                    entry->ImageName.Length / sizeof(wchar_t), &name[0], len, nullptr, nullptr);

                std::string proc_lower = Process_Name;
                std::string name_lower = name;
                for (auto& c : proc_lower) if (c >= 'A' && c <= 'Z') c += 32;
                for (auto& c : name_lower) if (c >= 'A' && c <= 'Z') c += 32;

                if (proc_lower == name_lower) {
                    Local_Process = static_cast<std::uint32_t>(reinterpret_cast<uintptr_t>(entry->UniqueProcessId));
                    m_ProcessId = Local_Process;
                    break;
                }
            }
        }
        if (!entry->NextEntryOffset) break;
        entry = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(
            reinterpret_cast<uint8_t*>(entry) + entry->NextEntryOffset);
    }

    return Local_Process;
}

std::uint64_t UsermodeMemory::Find_Module(const std::string& Module_Name)
{
    OBF_PROLOGUE;
    std::uint64_t Module_Address = 0;

    if (!m_ProcessHandle || m_ProcessHandle == INVALID_HANDLE_VALUE || !DriverNtOpenProcess) {
        OBF_JUNK_BLOCK;
        return Module_Address;
    }

    PROCESS_BASIC_INFORMATION pbi;
    ULONG ret_len = 0;
    auto nq_status = Api::NtQueryInformationProcess(m_ProcessHandle,
        ProcessBasicInformation, &pbi, sizeof(pbi), &ret_len);
    if (nq_status < 0 || !pbi.PebBaseAddress) {
        OBF_JUNK_BLOCK;
        return Module_Address;
    }

    uint64_t image_base = 0;
    if (DriverReadVirtualMemory) {
        DriverReadVirtualMemory(m_ProcessHandle,
            reinterpret_cast<PVOID>(reinterpret_cast<uint64_t>(pbi.PebBaseAddress) + 0x10),
            &image_base, sizeof(image_base), nullptr);
    }

    if (!image_base) {
        OBF_JUNK_BLOCK;
        return Module_Address;
    }

    if (_stricmp(Module_Name.c_str(), skCrypt("RobloxPlayerBeta.exe")) == 0) {
        m_BaseAddress = image_base;
        return image_base;
    }

    uint64_t ldr_addr = 0;
    if (DriverReadVirtualMemory) {
        DriverReadVirtualMemory(m_ProcessHandle,
            reinterpret_cast<PVOID>(reinterpret_cast<uint64_t>(pbi.PebBaseAddress) + 0x18),
            &ldr_addr, sizeof(ldr_addr), nullptr);
    }
    if (!ldr_addr) {
        OBF_JUNK_BLOCK;
        return Module_Address;
    }

    LIST_ENTRY module_list;
    if (DriverReadVirtualMemory) {
        DriverReadVirtualMemory(m_ProcessHandle,
            reinterpret_cast<PVOID>(ldr_addr + 0x30),
            &module_list, sizeof(module_list), nullptr);
    }

    LIST_ENTRY current = module_list;
    uint64_t current_flink = reinterpret_cast<uint64_t>(current.Flink);
    uint64_t head_flink = reinterpret_cast<uint64_t>(module_list.Flink);

    while (true) {
        LDR_DATA_TABLE_ENTRY entry_data;
        bool read_ok = false;
        if (DriverReadVirtualMemory) {
            read_ok = DriverReadVirtualMemory(m_ProcessHandle,
                reinterpret_cast<PVOID>(current_flink), &entry_data, sizeof(entry_data), nullptr) >= 0;
        }

        if (!read_ok || current_flink == 0) break;

        UNICODE_STRING& mod_name = entry_data.FullDllName;

        if (mod_name.Buffer && mod_name.Length > 0) {
            int len = WideCharToMultiByte(CP_UTF8, 0, mod_name.Buffer,
                mod_name.Length / sizeof(wchar_t), nullptr, 0, nullptr, nullptr);
            if (len > 0) {
                std::string name(len, '\0');
                WideCharToMultiByte(CP_UTF8, 0, mod_name.Buffer,
                    mod_name.Length / sizeof(wchar_t), &name[0], len, nullptr, nullptr);

                size_t pos = name.rfind('\\');
                std::string fname = (pos != std::string::npos) ? name.substr(pos + 1) : name;

                std::string mod_lower = Module_Name;
                std::string fname_lower = fname;
                for (auto& c : mod_lower) if (c >= 'A' && c <= 'Z') c += 32;
                for (auto& c : fname_lower) if (c >= 'A' && c <= 'Z') c += 32;

                if (mod_lower == fname_lower) {
                    Module_Address = reinterpret_cast<std::uint64_t>(entry_data.DllBase);
                    m_BaseAddress = Module_Address;
                    break;
                }
            }
        }

        if (current_flink == head_flink) break;
        uint64_t next_flink = 0;
        if (DriverReadVirtualMemory) {
            DriverReadVirtualMemory(m_ProcessHandle,
                reinterpret_cast<PVOID>(current_flink),
                &next_flink, sizeof(next_flink), nullptr);
        }
        current_flink = next_flink;
    }

    return Module_Address;
}

bool UsermodeMemory::Attach_Process(const std::string& Process_Name)
{
    OBF_PROLOGUE;

    if (!DriverNtOpenProcess || !m_ProcessId) {
        OBF_JUNK_BLOCK;
        return false;
    }

    CLIENT_ID cid;
    cid.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(m_ProcessId));
    cid.UniqueThread = nullptr;

    OBJECT_ATTRIBUTES oa;
    oa.Length = sizeof(oa);
    oa.RootDirectory = nullptr;
    oa.ObjectName = nullptr;
    oa.Attributes = 0;
    oa.SecurityDescriptor = nullptr;
    oa.SecurityQualityOfService = nullptr;

    HANDLE Process = nullptr;
    NTSTATUS status = DriverNtOpenProcess(&Process,
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
        &oa, &cid);

    if (status < 0 || !Process || Process == INVALID_HANDLE_VALUE) {
        OBF_JUNK_BLOCK;
        return false;
    }

    m_ProcessHandle = Process;
    return true;
}

void UsermodeMemory::Detach_Process()
{
    if (m_ProcessHandle && m_ProcessHandle != INVALID_HANDLE_VALUE) {
        Api::NtClose(m_ProcessHandle);
        m_ProcessHandle = nullptr;
    }
    m_ProcessId = 0;
    m_BaseAddress = 0;
}

std::string UsermodeMemory::Read_String(std::uint64_t Address)
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
    if (DriverReadVirtualMemory) {
        DriverReadVirtualMemory(m_ProcessHandle, reinterpret_cast<void*>(String_Address), Buffer.data(), (ULONG)Buffer.size(), nullptr);
    }

    return std::string(Buffer.data(), String_Length);
}

void UsermodeMemory::Write_String(std::uint64_t Address, const std::string& Value)
{
    OBF_PROLOGUE;
    auto Str = Read<RbxString>(Address);

    if (Value.length() > Str.Capacity)
    {
        while (Value.length() > Str.Capacity)
            Str.Capacity = Str.Capacity * 2 + 1;

        Str.Data.Pointer = reinterpret_cast<std::uint64_t>(
            Api::VirtualAllocEx(
                m_ProcessHandle,
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
        if (DriverWriteVirtualMemory) {
            DriverWriteVirtualMemory(
                m_ProcessHandle,
                reinterpret_cast<void*>(Str.Data.Pointer),
                (void*)Value.data(),
                (ULONG)Value.length(),
                nullptr
            );
        }
    }
    else
    {
        Str.Capacity = 15;
        Write<RbxString>(Address, Str);
        if (DriverWriteVirtualMemory) {
            DriverWriteVirtualMemory(
                m_ProcessHandle,
                reinterpret_cast<void*>(Address),
                (void*)Value.data(),
                (ULONG)Value.length(),
                nullptr
            );
        }
    }
}
