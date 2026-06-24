#pragma once
#include <Windows.h>
#include <stdint.h>
#include <string>
#include <Infrastructure/Logger.h>
#include <Miscellaneous/Protection/External/oxorany_include.h>

struct GdiFunctionTable {
    static const char** GetCandidates()
    {
        static const char* candidates[] = {
            "NtGdiDdDDIReclaimAllocations2",
            "NtGdiDdDDIReclaimAllocations",
            "NtGdiDdDDICreateSynchronizationObject",
            "NtGdiDdDDIWaitForVerticalBlankEvent",
            "NtGdiDdDDIGetScanLine",
            "NtGdiDdDDISetVidPnSourceAddress",
            "NtGdiDdDDIGetPresentHistory",
            "NtGdiDdDDICheckOcclusion",
            "NtGdiDdDDIGetPresentQueueEvent",
        };
        return candidates;
    }
    static constexpr int Count = 9;
};

class IDriverAbstraction {
public:
    virtual ~IDriverAbstraction() = default;

    virtual bool Load() = 0;
    virtual void Unload() = 0;
    virtual HANDLE GetHandle() const = 0;

    virtual bool PhysRead(uint64_t phys_addr, void* buffer, uint32_t size) = 0;
    virtual bool PhysWrite(uint64_t phys_addr, const void* buffer, uint32_t size) = 0;

    virtual bool ReadMemory(uint64_t virt_addr, void* buffer, uint64_t size) = 0;
    virtual bool WriteMemory(uint64_t virt_addr, const void* buffer, uint64_t size) = 0;
    virtual bool WriteToReadOnlyMemory(uint64_t address, const void* buffer, uint32_t size) = 0;
    virtual bool SetMemory(uint64_t address, uint32_t value, uint64_t size) = 0;

    virtual uint64_t AllocatePool(uint32_t pool_type, uint64_t size) = 0;
    virtual bool FreePool(uint64_t address) = 0;

    virtual uint64_t GetKernelModuleExport(uint64_t kernel_module_base, const std::string& function_name) = 0;

    template<typename T, typename ...A>
    bool CallKernelFunction(T* out_result, uint64_t kernel_function_address, const A ...arguments)
    {
        constexpr auto call_void = std::is_same_v<T, void>;

        if constexpr (!call_void)
        {
            if (!out_result)
                return false;
        }
        else
        {
            UNREFERENCED_PARAMETER(out_result);
        }

        if (!kernel_function_address)
            return false;

        static void* cached_user_function = nullptr;
        static const char* cached_function_name = nullptr;

        if (!cached_user_function)
        {
            const char* dlls[] = { "win32u.dll", "gdi32full.dll" };
            const char** candidates = GdiFunctionTable::GetCandidates();
            for (int fi = 0; fi < GdiFunctionTable::Count; ++fi)
            {
                const char* funcName = candidates[fi];
                for (auto dllName : dlls)
                {
                    HMODULE hMod = GetModuleHandleA(dllName);
                    if (!hMod)
                        hMod = LoadLibraryA(dllName);
                    if (hMod)
                    {
                        void* addr = reinterpret_cast<void*>(GetProcAddress(hMod, funcName));
                        if (addr)
                        {
                            Logger::Log(WRAPPER_MARCO("[CallKernelFunction] user-mode found"));
                            Logger::Log(funcName);
                            Logger::Log(WRAPPER_MARCO("in"));
                            Logger::Log(dllName);
                            cached_user_function = addr;
                            cached_function_name = funcName;
                            break;
                        }
                    }
                }
                if (cached_user_function)
                    break;
            }

            if (!cached_user_function)
            {
                Logger::Log(WRAPPER_MARCO("[CallKernelFunction] FAIL GetProcAddress all candidates (win32u + gdi32full)"));
                Logger::Flush();
                return false;
            }
        }

        Logger::Log(WRAPPER_MARCO("[CallKernelFunction] user addr OK, resolving kernel side for"));
        Logger::Log(cached_function_name);

        uint64_t kernel_function_ptr = 0;
        uint64_t kernel_original_function_address = 0;

        Logger::Log(WRAPPER_MARCO("[CallKernelFunction] GetGdiKernelInfo..."));
        if (!GetGdiKernelInfo(cached_function_name, &kernel_function_ptr, &kernel_original_function_address))
        {
            Logger::Log(WRAPPER_MARCO("[CallKernelFunction] FAIL GetGdiKernelInfo"));
            return false;
        }
        Logger::LogHex(WRAPPER_MARCO("[CallKernelFunction] kernel_function_ptr"), kernel_function_ptr);
        Logger::LogHex(WRAPPER_MARCO("[CallKernelFunction] kernel_original_function"), kernel_original_function_address);

        Logger::Log(WRAPPER_MARCO("[CallKernelFunction] patching kernel ptr..."));
        if (!WriteToReadOnlyMemory(kernel_function_ptr, &kernel_function_address, sizeof(kernel_function_address)))
        {
            Logger::Log(WRAPPER_MARCO("[CallKernelFunction] FAIL WriteToReadOnlyMemory (patch)"));
            Logger::Flush();
            return false;
        }
        Logger::Log(WRAPPER_MARCO("[CallKernelFunction] kernel ptr patched, calling function..."));

        Logger::Flush();

        if constexpr (!call_void)
        {
            using FunctionFn = T(__stdcall*)(A...);
            const auto Function = static_cast<FunctionFn>(cached_user_function);
            *out_result = Function(arguments...);
        }
        else
        {
            using FunctionFn = void(__stdcall*)(A...);
            const auto Function = static_cast<FunctionFn>(cached_user_function);
            Function(arguments...);
        }

        Logger::Log(WRAPPER_MARCO("[CallKernelFunction] function returned, restoring kernel ptr..."));
        WriteToReadOnlyMemory(kernel_function_ptr, &kernel_original_function_address, sizeof(kernel_original_function_address));
        Logger::Log(WRAPPER_MARCO("[CallKernelFunction] SUCCESS"));
        return true;
    }

    virtual bool ClearMmUnloadedDrivers() = 0;

protected:
    virtual bool GetGdiKernelInfo(const char* function_name, uint64_t* out_kernel_function_ptr, uint64_t* out_kernel_original_function_address) = 0;

    virtual uint64_t VirtToPhys(uint64_t virt_addr) = 0;
};
