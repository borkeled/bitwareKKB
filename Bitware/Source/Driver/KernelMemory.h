#pragma once
#include <windows.h>
#include <string>
#include "MemoryInterface.h"

class KernelMemory final : public MemoryInterface {
public:
    KernelMemory();
    ~KernelMemory() override;

    bool Connect();

    std::uint32_t Find_Process(const std::string& name) override;
    std::uint64_t Find_Module(const std::string& name) override;
    bool Attach_Process(const std::string& name) override;
    void Detach_Process() override;

    bool ReadRaw(std::uint64_t addr, void* buf, size_t size) override;
    bool WriteRaw(std::uint64_t addr, const void* buf, size_t size) override;

    std::string Read_String(std::uint64_t addr) override;
    void        Write_String(std::uint64_t addr, const std::string& val) override;

    bool ReadKeyboardInput(BITWARE_KEYBOARD_DATA* buffer, ULONG* count) override;

    std::uint32_t Get_Process() const override { return m_ProcessId; }
    std::uint64_t Get_Module()  const override { return m_BaseAddress; }
    HANDLE   Get_Handle()  const override { return m_ProcessHandle; }

    void Shutdown() override;

private:
    HANDLE m_DriverHandle = INVALID_HANDLE_VALUE;
    DWORD m_IoctlBase = 0;
    DWORD m_IoctlReadMemory = 0;
    DWORD m_IoctlWriteMemory = 0;
    DWORD m_IoctlFindProcess = 0;
    DWORD m_IoctlFindModule = 0;
    DWORD m_IoctlUnload = 0;
    DWORD m_IoctlReadInput = 0;

    bool SendIoctl(DWORD code, const void* in_buf, DWORD in_size, void* out_buf, DWORD out_size, LPDWORD bytesReturned = nullptr);
};
