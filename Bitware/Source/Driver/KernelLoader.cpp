#include "KernelLoader.h"
#include <Auth/skStr.h>
#include <Driver/kdmapper/kdmapper.hpp>
#include <Driver/kdmapper/intel_driver.hpp>
#include <Driver/kdmapper/bitware_driver_resource.hpp>
#include "IoctlDefs.h"

namespace KernelLoader
{
    static HANDLE g_IntelDeviceHandle = nullptr;
    static bool g_DriverLoaded = false;

    static std::wstring BuildDevicePath()
    {
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        ULONG seed = ft.dwLowDateTime ^ ft.dwHighDateTime;
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
            CloseHandle(device);
            g_DriverLoaded = true;
            return true;
        }

        g_IntelDeviceHandle = intel_driver::Load();
        if (!g_IntelDeviceHandle || g_IntelDeviceHandle == INVALID_HANDLE_VALUE)
        {
            g_IntelDeviceHandle = nullptr;
            return false;
        }

        std::vector<uint8_t> driverData(
            bitware_driver_resource::driver,
            bitware_driver_resource::driver + bitware_driver_resource::size
        );

        uint64_t result = kdmapper::MapDriver(g_IntelDeviceHandle, driverData);

        if (!result)
        {
            intel_driver::Unload(g_IntelDeviceHandle);
            g_IntelDeviceHandle = nullptr;
            return false;
        }

        intel_driver::Unload(g_IntelDeviceHandle);
        g_IntelDeviceHandle = nullptr;

        Sleep(200);

        device = CreateFileW(
            path.c_str(),
            GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr
        );

        if (device != INVALID_HANDLE_VALUE && device != nullptr)
        {
            CloseHandle(device);
            g_DriverLoaded = true;
            return true;
        }

        return false;
    }

    void UnloadDriver()
    {
        g_IntelDeviceHandle = nullptr;
        g_DriverLoaded = false;
    }
}
