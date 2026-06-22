#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include <windows.h>
#include "IoctlDefs.h"

union RbxStringData {
    char Inline[16];
    std::uint64_t Pointer;
};

struct RbxString {
    RbxStringData Data;
    std::uint64_t Length;
    std::uint64_t Capacity;
};

class MemoryInterface {
public:
    virtual ~MemoryInterface() = default;

    virtual uint32_t Find_Process(const std::string& name) = 0;
    virtual bool     Attach_Process(const std::string& name) = 0;
    virtual void     Detach_Process() = 0;

    virtual uint64_t Find_Module(const std::string& name) = 0;

    virtual bool ReadRaw(uint64_t addr, void* buf, size_t size) = 0;
    virtual bool WriteRaw(uint64_t addr, const void* buf, size_t size) = 0;

    template<typename T>
    T Read(uint64_t addr) {
        T val{};
        ReadRaw(addr, &val, sizeof(T));
        return val;
    }

    template<typename T>
    void Write(uint64_t addr, const T& val) {
        WriteRaw(addr, &val, sizeof(T));
    }

    virtual std::string Read_String(uint64_t addr) = 0;
    virtual void        Write_String(uint64_t addr, const std::string& val) = 0;

    virtual uint32_t Get_Process() const = 0;
    virtual uint64_t Get_Module()  const = 0;
    virtual HANDLE   Get_Handle()  const = 0;

    virtual bool ReadKeyboardInput(BITWARE_KEYBOARD_DATA* buffer, ULONG* count) { return false; }

    virtual void Shutdown() {}

protected:
    uint32_t m_ProcessId = 0;
    uint64_t m_BaseAddress = 0;
    HANDLE   m_ProcessHandle = nullptr;
};

inline std::unique_ptr<MemoryInterface> g_Memory;
