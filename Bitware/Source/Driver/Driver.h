#pragma once
#include <windows.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <memory>
#include <Infrastructure/ApiHiding.h>
#include <Infrastructure/Obfuscation.h>
#include "MemoryInterface.h"

using ReadVirtualMemoryFn = intptr_t(NTAPI*)(HANDLE, PVOID, PVOID, ULONG, PULONG);
using WriteVirtualMemoryFn = intptr_t(NTAPI*)(HANDLE, PVOID, PVOID, ULONG, PULONG);
using OpenProcessFn = NTSTATUS(NTAPI*)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, CLIENT_ID*);
using QuerySysInfoFn = NTSTATUS(NTAPI*)(ULONG, PVOID, ULONG, PULONG);

inline ReadVirtualMemoryFn DriverReadVirtualMemory = nullptr;
inline WriteVirtualMemoryFn DriverWriteVirtualMemory = nullptr;
inline OpenProcessFn DriverNtOpenProcess = nullptr;
inline QuerySysInfoFn DriverNtQuerySystemInformation = nullptr;

using WriteMousePosFn = void(NTAPI*)(HANDLE, PVOID, float, float);
inline WriteMousePosFn DriverWriteMousePosition = nullptr;

class UsermodeMemory final : public MemoryInterface {
public:
    UsermodeMemory() = default;
    ~UsermodeMemory() override;

    std::uint32_t Find_Process(const std::string& Process_Name) override;
    std::uint64_t Find_Module(const std::string& Module_Name) override;
    bool Attach_Process(const std::string& Process_Name) override;
    void Detach_Process() override;

    bool ReadRaw(std::uint64_t Address, void* Buffer, size_t Size) override;
    bool WriteRaw(std::uint64_t Address, const void* Buffer, size_t Size) override;

    std::string Read_String(std::uint64_t Address) override;
    void Write_String(std::uint64_t Address, const std::string& Value) override;

    bool ReadKeyboardInput(BITWARE_KEYBOARD_DATA* buffer, ULONG* count) override;

    std::uint32_t Get_Process() const override { return m_ProcessId; }
    std::uint64_t Get_Module()  const override { return m_BaseAddress; }
    HANDLE   Get_Handle()  const override { return m_ProcessHandle; }
};
