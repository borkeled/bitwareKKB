#include "Driver.h"
#include <Auth/skStr.h>
#include <psapi.h>

UsermodeMemory::~UsermodeMemory()
{
    Detach_Process();
}

bool UsermodeMemory::ReadKeyboardInput(BITWARE_KEYBOARD_DATA* buffer, ULONG* count)
{
    UNREFERENCED_PARAMETER(buffer);
    if (count) *count = 0;
    return false;
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

    HANDLE snapshot = Api::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return Local_Process;

    PROCESSENTRY32 pe = { sizeof(pe) };
    if (Api::Process32First(snapshot, &pe))
    {
        do {
            std::string name_lower = pe.szExeFile;
            std::string proc_lower = Process_Name;
            for (auto& c : proc_lower) if (c >= 'A' && c <= 'Z') c += 32;
            for (auto& c : name_lower) if (c >= 'A' && c <= 'Z') c += 32;

            if (proc_lower == name_lower)
            {
                Local_Process = pe.th32ProcessID;
                m_ProcessId = Local_Process;
                break;
            }
        } while (Api::Process32Next(snapshot, &pe));
    }

    Api::CloseHandle(snapshot);
    return Local_Process;
}

std::uint64_t UsermodeMemory::Find_Module(const std::string& Module_Name)
{
    OBF_PROLOGUE;
    std::uint64_t Module_Address = 0;

    if (!m_ProcessId)
        return Module_Address;

    HANDLE snapshot = Api::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_ProcessId);
    if (snapshot == INVALID_HANDLE_VALUE)
        return Module_Address;

    MODULEENTRY32 me = { sizeof(me) };
    if (Api::Module32First(snapshot, &me))
    {
        do {
            std::string mod_lower = Module_Name;
            std::string name_lower = me.szModule;
            for (auto& c : mod_lower) if (c >= 'A' && c <= 'Z') c += 32;
            for (auto& c : name_lower) if (c >= 'A' && c <= 'Z') c += 32;

            if (mod_lower == name_lower)
            {
                Module_Address = reinterpret_cast<std::uint64_t>(me.modBaseAddr);
                m_BaseAddress = Module_Address;
                break;
            }
        } while (Api::Module32Next(snapshot, &me));
    }

    Api::CloseHandle(snapshot);
    return Module_Address;
}

bool UsermodeMemory::Attach_Process(const std::string& Process_Name)
{
    OBF_PROLOGUE;

    if (!DriverNtOpenProcess || !m_ProcessId) {
        OBF_JUNK_BLOCK;
        return false;
    }

    OBF_OPAQUE_TRUE
    {
        volatile ULONG_PTR junk = 0;
        junk ^= junk;
        OBF_JUNK_BLOCK;
    }

    CLIENT_ID cid;
    cid.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(m_ProcessId));
    cid.UniqueThread = nullptr;

    OBJECT_ATTRIBUTES oa;
    oa.Length = sizeof(oa);
    oa.RootDirectory = nullptr;
    oa.ObjectName = nullptr;
    oa.Attributes = OBJ_KERNEL_HANDLE;
    oa.SecurityDescriptor = nullptr;
    oa.SecurityQualityOfService = nullptr;

    ACCESS_MASK desiredAccess = PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION;

    HANDLE Process = nullptr;
    NTSTATUS status = DriverNtOpenProcess(&Process, desiredAccess, &oa, &cid);

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
        Api::CloseHandle(m_ProcessHandle);
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
