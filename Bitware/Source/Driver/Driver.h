#pragma once
#include <windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <memory>
#include <Infrastructure/ApiHiding.h>
#include <Infrastructure/Obfuscation.h>

extern "C" intptr_t Driver_ReadVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    ULONG NumberOfBytesToRead,
    PULONG NumberOfBytesRead
);

extern "C" intptr_t Driver_WriteVirtualMemory(
    HANDLE ProcessHandle,
    PVOID BaseAddress,
    PVOID Buffer,
    ULONG NumberOfBytesToWrite,
    PULONG NumberOfBytesWritten
);

extern "C" void Driver_WriteMousePosition(
    HANDLE ProcessHandle,
    PVOID InputObjectAddress,
    float X,
    float Y
);

union RbxStringData {
    char Inline[16];
    std::uint64_t Pointer;
};

struct RbxString {
    RbxStringData Data;
    std::uint64_t Length;
    std::uint64_t Capacity;
};

class Driver_t final {
public:
    Driver_t() = default;
    ~Driver_t() = default;

    std::uint32_t Find_Process(const std::string& Process_Name);
    std::uint64_t Find_Module(const std::string& Module_Name);
    bool Attach_Process(const std::string& Process_Name);

    std::string Read_String(std::uint64_t Address);
    void Write_String(std::uint64_t Address, const std::string& Value);

    template <typename T>
    T Read(std::uint64_t Address);

    template <typename T>
    void Write(std::uint64_t Address, T Value);

    std::uint32_t Get_Process();
    std::uint64_t Get_Module();
    HANDLE Get_Handle();

private:
    std::uint32_t Process_ID;
    std::uint64_t Base_Address;
    HANDLE Process_Handle;
};

template <typename T>
T Driver_t::Read(std::uint64_t Address) {
    OBF_PROLOGUE;
    T Buffer{};
    Driver_ReadVirtualMemory(Process_Handle, reinterpret_cast<void*>(Address), &Buffer, sizeof(T), nullptr);
    OBF_OPAQUE_TRUE { OBF_JUNK_BLOCK; }
    return Buffer;
}

template <typename T>
void Driver_t::Write(std::uint64_t Address, T Value) {
    OBF_PROLOGUE;
    OBF_JUNK_DECLARE;
    Driver_WriteVirtualMemory(Process_Handle, reinterpret_cast<void*>(Address), &Value, sizeof(T), nullptr);
}

inline std::unique_ptr<Driver_t> Driver = std::make_unique<Driver_t>();
