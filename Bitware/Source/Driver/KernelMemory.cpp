#include "KernelMemory.h"
#include <Auth/skStr.h>
#include <Infrastructure/Obfuscation.h>
#include <windows.h>

KernelMemory::KernelMemory()
{
}

KernelMemory::~KernelMemory()
{
    Detach_Process();
    if (m_DriverHandle != INVALID_HANDLE_VALUE && m_DriverHandle != nullptr)
    {
        CloseHandle(m_DriverHandle);
        m_DriverHandle = INVALID_HANDLE_VALUE;
    }
}

bool KernelMemory::Connect()
{
    OBF_PROLOGUE;

    m_DriverHandle = CreateFileA(
        skCrypt("\\\\.\\BitwareDevice"),
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr
    );

    return m_DriverHandle != INVALID_HANDLE_VALUE && m_DriverHandle != nullptr;
}

bool KernelMemory::SendIoctl(DWORD code, const void* in_buf, DWORD in_size, void* out_buf, DWORD out_size)
{
    if (m_DriverHandle == INVALID_HANDLE_VALUE || !m_DriverHandle)
        return false;

    DWORD bytes_returned = 0;
    return DeviceIoControl(
        m_DriverHandle, code,
        const_cast<void*>(in_buf), in_size,
        out_buf, out_size,
        &bytes_returned, nullptr
    ) != FALSE;
}

std::uint32_t KernelMemory::Find_Process(const std::string& name)
{
    OBF_PROLOGUE;
    wchar_t wide_name[260]{};
    size_t converted = 0;
    mbstowcs_s(&converted, wide_name, name.c_str(), 259);

    uint32_t pid = 0;
    if (SendIoctl(0x80002003, wide_name, (DWORD)(wcslen(wide_name) * 2 + 2), &pid, sizeof(pid)))
    {
        m_ProcessId = pid;
    }
    return m_ProcessId;
}

std::uint64_t KernelMemory::Find_Module(const std::string& name)
{
    OBF_PROLOGUE;
    struct {
        uint32_t pid;
        wchar_t name[260];
    } input{};
    input.pid = m_ProcessId;
    size_t converted = 0;
    mbstowcs_s(&converted, input.name, name.c_str(), 259);

    uint64_t base = 0;
    if (SendIoctl(0x80002004, &input, sizeof(input), &base, sizeof(base)))
    {
        m_BaseAddress = base;
    }
    return m_BaseAddress;
}

bool KernelMemory::Attach_Process(const std::string& name)
{
    OBF_PROLOGUE;
    uint32_t pid = Find_Process(name);
    if (!pid) return false;
    m_ProcessId = pid;
    return pid != 0;
}

void KernelMemory::Detach_Process()
{
    m_ProcessId = 0;
    m_BaseAddress = 0;
}

bool KernelMemory::ReadRaw(std::uint64_t addr, void* buf, size_t size)
{
    OBF_PROLOGUE;
    struct {
        uint32_t pid;
        uint64_t address;
        uint32_t size;
    } input{};
    input.pid = m_ProcessId;
    input.address = addr;
    input.size = (uint32_t)size;

    return SendIoctl(0x80002001, &input, sizeof(input), buf, (DWORD)size);
}

struct KernelWriteInput {
    uint32_t pid;
    uint64_t address;
    uint32_t size;
    char data[1];
};

bool KernelMemory::WriteRaw(std::uint64_t addr, const void* buf, size_t size)
{
    OBF_PROLOGUE;
    KernelWriteInput* input = (KernelWriteInput*)alloca(sizeof(KernelWriteInput) + size);

    input->pid = m_ProcessId;
    input->address = addr;
    input->size = (uint32_t)size;
    memcpy(input->data, buf, size);

    return SendIoctl(0x80002002, input, (DWORD)(sizeof(KernelWriteInput) + size - 1), nullptr, 0);
}

std::string KernelMemory::Read_String(std::uint64_t addr)
{
    OBF_PROLOGUE;
    std::int32_t len = Read<std::int32_t>(addr + 0x10);
    if (len <= 0 || len > 255)
    {
        OBF_JUNK_BLOCK;
        return std::string(skCrypt("Unknown"));
    }

    uint64_t str_addr = (len >= 16) ? Read<std::uint64_t>(addr) : addr;

    std::string result(len, '\0');
    ReadRaw(str_addr, result.data(), len);
    return result;
}

void KernelMemory::Write_String(std::uint64_t addr, const std::string& val)
{
    OBF_PROLOGUE;
    auto Str = Read<RbxString>(addr);

    if (val.length() > Str.Capacity)
        return;

    Str.Length = val.length();

    if (Str.Length > 15)
    {
        Write<RbxString>(addr, Str);
        WriteRaw(Str.Data.Pointer, val.data(), val.length());
    }
    else
    {
        Str.Capacity = 15;
        Write<RbxString>(addr, Str);
        WriteRaw(addr, val.data(), val.length());
    }
}
