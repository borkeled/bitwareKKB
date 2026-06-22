#include "KernelLoader.h"
#include <Auth/skStr.h>
#include <Infrastructure/Logger.h>
#include <Driver/kdmapper/kdmapper.hpp>
#include <Driver/kdmapper/intel_driver.hpp>
#include <Driver/kdmapper/bitware_driver_resource.hpp>

namespace KernelLoader
{
    static HANDLE g_IntelDeviceHandle = nullptr;
    static bool g_DriverLoaded = false;

    bool LoadDriver()
    {
        if (g_DriverLoaded)
            return true;

        HANDLE device = CreateFileA(
            skCrypt("\\\\.\\BitwareDevice"),
            GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr
        );

        if (device != INVALID_HANDLE_VALUE && device != nullptr)
        {
            CloseHandle(device);
            Logger::Log(skCrypt("[KernelLoader] driver already loaded"));
            g_DriverLoaded = true;
            return true;
        }

        Logger::Log(skCrypt("[KernelLoader] loading vulnerable Intel driver"));

        g_IntelDeviceHandle = intel_driver::Load();
        if (!g_IntelDeviceHandle || g_IntelDeviceHandle == INVALID_HANDLE_VALUE)
        {
            Logger::Log(skCrypt("[KernelLoader] failed to load Intel vulnerable driver"));
            g_IntelDeviceHandle = nullptr;
            return false;
        }

        Logger::Log(skCrypt("[KernelLoader] mapping BitwareDrv via kdmapper"));

        std::vector<uint8_t> driverData(
            bitware_driver_resource::driver,
            bitware_driver_resource::driver + bitware_driver_resource::size
        );

        uint64_t result = kdmapper::MapDriver(g_IntelDeviceHandle, driverData);

        if (!result)
        {
            Logger::Log(skCrypt("[KernelLoader] kdmapper mapping failed"));
            intel_driver::Unload(g_IntelDeviceHandle);
            g_IntelDeviceHandle = nullptr;
            return false;
        }

        Logger::LogHex(skCrypt("[KernelLoader] driver mapped at"), result);

        intel_driver::Unload(g_IntelDeviceHandle);
        g_IntelDeviceHandle = nullptr;

        Sleep(200);

        device = CreateFileA(
            skCrypt("\\\\.\\BitwareDevice"),
            GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr
        );

        if (device != INVALID_HANDLE_VALUE && device != nullptr)
        {
            CloseHandle(device);
            Logger::Log(skCrypt("[KernelLoader] BitwareDevice connected"));
            g_DriverLoaded = true;
            return true;
        }

        Logger::Log(skCrypt("[KernelLoader] BitwareDevice not found after mapping"));
        return false;
    }

    void UnloadDriver()
    {
        g_DriverLoaded = false;
    }
}
