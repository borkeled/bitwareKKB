#include "KernelLoader.h"
#include <memory>
#include <Auth/skStr.h>
#include <Driver/kdmapper/kdmapper.hpp>
#include <Driver/kdmapper/bitware_driver_resource.hpp>
#include <Driver/DriverAbstraction/EneIo64Driver.h>
#include <Infrastructure/Logger.h>
#include <Miscellaneous/Protection/External/oxorany_include.h>
#include "IoctlDefs.h"

namespace KernelLoader
{
    static bool g_DriverLoaded = false;

    static std::wstring BuildDevicePath()
    {
        ULONG seed = kBitwareSeed;
        WCHAR name[5];
        IoctlDerivation::DeriveDeviceName(seed, name, 5);
        return L"\\\\.\\" + std::wstring(name);
    }

    bool LoadDriver()
    {
        if (g_DriverLoaded)
            return true;

        std::wstring path = BuildDevicePath();

        HANDLE device = CreateFileW(
            path.c_str(),
            GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr
        );

        if (device != INVALID_HANDLE_VALUE && device != nullptr)
        {
            Logger::Log(WRAPPER_MARCO("[KernelLoader] bitware device already exists (driver already loaded)"));
            CloseHandle(device);
            g_DriverLoaded = true;
            return true;
        }

        Logger::Log(WRAPPER_MARCO("[KernelLoader] creating EneIo64Driver..."));
        auto temp_driver = std::make_unique<EneIo64Driver>();
        if (!temp_driver->Load())
        {
            return false;
        }

        std::vector<uint8_t> driverData(
            bitware_driver_resource::driver,
            bitware_driver_resource::driver + bitware_driver_resource::size
        );

        Logger::Log(WRAPPER_MARCO("[KernelLoader] calling kdmapper::MapDriver..."));
        uint64_t result = kdmapper::MapDriver(temp_driver.get(), driverData);
        Logger::LogHex(WRAPPER_MARCO("[KernelLoader] kdmapper::MapDriver result"), result);

        if (!result)
        {
            Logger::Log(WRAPPER_MARCO("[KernelLoader] FAIL MapDriver, unloading eneio..."));
            temp_driver->Unload();
            return false;
        }

        temp_driver->Unload();
        temp_driver.reset();

        Logger::Log(WRAPPER_MARCO("[KernelLoader] opening bitware device after unload..."));
        
        constexpr int max_retries = 10;
        constexpr DWORD retry_delay_ms = 100;
        bool opened_successfully = false;

        for (int retry = 0; retry < max_retries; ++retry)
        {
            device = CreateFileW(
                path.c_str(),
                GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr
            );

            if (device != INVALID_HANDLE_VALUE && device != nullptr)
            {
                opened_successfully = true;
                break;
            }
            Sleep(retry_delay_ms);
        }

        if (opened_successfully)
        {
            Logger::Log(WRAPPER_MARCO("[KernelLoader] bitware device opened OK"));
            CloseHandle(device);
            g_DriverLoaded = true;
            return true;
        }

        Logger::LogHex(WRAPPER_MARCO("[KernelLoader] FAIL open bitware device, error"), GetLastError());
        Logger::Flush();
        return false;
    }

    void UnloadDriver()
    {
        g_DriverLoaded = false;
    }
}
