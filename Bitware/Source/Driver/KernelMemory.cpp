#include "KernelMemory.h"
#include "KernelLoader.h"
#include <Auth/skStr.h>
#include <Infrastructure/Obfuscation.h>
#include "IoctlDefs.h"

KernelMemory::KernelMemory()
{
}

KernelMemory::~KernelMemory()
{
    Shutdown();
}

ULONG KernelMemory::ComputeSharedSeed()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return ft.dwLowDateTime ^ ft.dwHighDateTime;
}

std::wstring KernelMemory::BuildDevicePath()
{
    ULONG seed = ComputeSharedSeed();
    WCHAR name[5];
    IoctlDerivation::DeriveDeviceName(seed, name, 5);
    return L"\\\\.\\" + std::wstring(name);
}

bool KernelMemory::Connect()
{
    OBF_PROLOGUE;

    if (!KernelLoader::LoadDriver())
    {
        return false;
    }

    std::wstring path = BuildDevicePath();
    m_DriverHandle = CreateFileW(
        path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING, 0, nullptr
    );

    if (m_DriverHandle == INVALID_HANDLE_VALUE || m_DriverHandle == nullptr)
        return false;

    ULONG seed = ComputeSharedSeed();
    m_IoctlBase = IoctlDerivation::DeriveIoctlBase(seed);

    return true;
}

bool KernelMemory::SendIoctl(DWORD code, const void* in_buf, DWORD in_size, void* out_buf, DWORD out_size, LPDWORD bytesReturned)
{
    if (m_DriverHandle == INVALID_HANDLE_VALUE || !m_DriverHandle)
        return false;

    DWORD bytes_returned = 0;
    BOOL result = DeviceIoControl(
        m_DriverHandle, code,
        const_cast<void*>(in_buf), in_size,
        out_buf, out_size,
        &bytes_returned, nullptr
    );
    if (bytesReturned)
        *bytesReturned = bytes_returned;
    return result != FALSE;
}

std::uint32_t KernelMemory::Find_Process(const std::string& name)
{
    OBF_PROLOGUE;
    BITWARE_FIND_PROCESS_INPUT input{};
    size_t converted = 0;
    mbstowcs_s(&converted, input.Name, name.c_str(), 259);

    uint32_t pid = 0;
    if (SendIoctl(m_IoctlBase + BITWARE_IOCTL_FIND_PROCESS, &input, sizeof(input), &pid, sizeof(pid)))
    {
        m_ProcessId = pid;
    }
    return m_ProcessId;
}

std::uint64_t KernelMemory::Find_Module(const std::string& name)
{
    OBF_PROLOGUE;
    BITWARE_FIND_MODULE_INPUT input{};
    input.ProcessId = m_ProcessId;
    size_t converted = 0;
    mbstowcs_s(&converted, input.Name, name.c_str(), 259);

    uint64_t base = 0;
    if (SendIoctl(m_IoctlBase + BITWARE_IOCTL_FIND_MODULE, &input, sizeof(input), &base, sizeof(base)))
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

void KernelMemory::Shutdown()
{
    if (m_DriverHandle != INVALID_HANDLE_VALUE && m_DriverHandle != nullptr)
    {
        DWORD bytesReturned = 0;
        DeviceIoControl(m_DriverHandle, m_IoctlBase + BITWARE_IOCTL_UNLOAD, nullptr, 0, nullptr, 0, &bytesReturned, nullptr);
        CloseHandle(m_DriverHandle);
        m_DriverHandle = INVALID_HANDLE_VALUE;
    }
    KernelLoader::UnloadDriver();
    Detach_Process();
}

bool KernelMemory::ReadRaw(std::uint64_t addr, void* buf, size_t size)
{
    OBF_PROLOGUE;
    BITWARE_READ_INPUT input{};
    input.ProcessId = m_ProcessId;
    input.Address = addr;
    input.Size = (ULONG)size;

    return SendIoctl(m_IoctlBase + BITWARE_IOCTL_READ_MEMORY, &input, sizeof(input), buf, (DWORD)size);
}

bool KernelMemory::WriteRaw(std::uint64_t addr, const void* buf, size_t size)
{
    OBF_PROLOGUE;
    ULONG totalSize = sizeof(BITWARE_WRITE_INPUT) + (ULONG)size;
    void* ioBuffer = alloca(totalSize);

    PBITWARE_WRITE_INPUT input = (PBITWARE_WRITE_INPUT)ioBuffer;
    input->ProcessId = m_ProcessId;
    input->Address = addr;
    input->Size = (ULONG)size;

    memcpy((unsigned char*)ioBuffer + sizeof(BITWARE_WRITE_INPUT), buf, size);

    return SendIoctl(m_IoctlBase + BITWARE_IOCTL_WRITE_MEMORY, ioBuffer, totalSize, nullptr, 0);
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
