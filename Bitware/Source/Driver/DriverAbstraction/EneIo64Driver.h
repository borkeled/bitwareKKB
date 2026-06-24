#pragma once
#include "IDriverAbstraction.h"
#include <string>
#include <vector>

class EneIo64Driver final : public IDriverAbstraction {
public:
    EneIo64Driver();
    ~EneIo64Driver() override;

    bool Load() override;
    void Unload() override;
    HANDLE GetHandle() const override { return m_DriverHandle; }

    bool PhysRead(uint64_t phys_addr, void* buffer, uint32_t size) override;
    bool PhysWrite(uint64_t phys_addr, const void* buffer, uint32_t size) override;

    bool ReadMemory(uint64_t virt_addr, void* buffer, uint64_t size) override;
    bool WriteMemory(uint64_t virt_addr, const void* buffer, uint64_t size) override;
    bool WriteToReadOnlyMemory(uint64_t address, const void* buffer, uint32_t size) override;
    bool SetMemory(uint64_t address, uint32_t value, uint64_t size) override;

    uint64_t AllocatePool(uint32_t pool_type, uint64_t size) override;
    bool FreePool(uint64_t address) override;

    uint64_t GetKernelModuleExport(uint64_t kernel_module_base, const std::string& function_name) override;

    bool ClearMmUnloadedDrivers() override;

    static constexpr uint32_t IOCTL_MAP_PHYS_TO_LIN = 0x80102040;
    static constexpr uint32_t IOCTL_UNMAP_PHYS_ADDR = 0x80102044;
    static constexpr const wchar_t* DEVICE_PATH = L"\\\\.\\GLCKIo";
    static constexpr uint64_t kInitialMapSize = 0x4000000;

    struct PHYS_MAP_INPUT {
        uint64_t Size;
        uint64_t Unknown1;
        uint64_t Unknown2;
        uint64_t MappingAddress;
        uint64_t Unknown3;
    };

protected:
    bool GetGdiKernelInfo(const char* function_name, uint64_t* out_kernel_function_ptr, uint64_t* out_kernel_original_function_address) override;
    uint64_t VirtToPhys(uint64_t virt_addr) override;

private:
    HANDLE m_DriverHandle = INVALID_HANDLE_VALUE;
    void* m_MappedMemoryBase = nullptr;
    uint64_t m_MappedSize = 0;
    uint64_t m_TotalPhysicalMemory = 0;
    uint64_t m_CachedCR3 = 0;
    uint64_t m_KernelCR3 = 0;
    uint64_t m_NtoskrnlBase = 0;
    uint64_t m_GdiKernelPtr = 0;
    uint64_t m_GdiOriginalFunction = 0;
    bool m_HasGdiInfo = false;
    std::string m_GdiFunctionName;

    void DumpModuleExports(const char* mod_name, uint64_t kernel_module_base, uint32_t max_names);
    uint64_t VirtToPhysWithPML4(uint64_t cr3, uint64_t virt_addr, bool five_level);
    uint64_t FindKernelCR3();

    bool GrowMapping(uint64_t needed_up_to);
    void UnmapPhysicalMemory();
    bool DiscoverCR3();
    uint64_t GetKernelBase();
    uint64_t FindEntryPointRva();
};
