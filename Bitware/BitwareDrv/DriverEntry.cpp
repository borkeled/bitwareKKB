#include <ntddk.h>
#include "BitwareDrv.h"

PDEVICE_OBJECT g_DeviceObject = NULL;
ULONG g_PoolTag = 0;
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

static ULONG ComputeSharedSeed()
{
    return kBitwareSeed;
}

static VOID BuildDeviceName(PWCHAR buf, ULONG maxLen)
{
    ULONG seed = ComputeSharedSeed();
    WCHAR core[5];
    IoctlDerivation::DeriveDeviceName(seed, core, 5);

    WCHAR prefixDev[] = { L'\\', L'D', L'e', L'v', L'i', L'c', L'e', L'\\', 0 };

    ULONG pos = 0;
    for (ULONG i = 0; prefixDev[i] && pos < maxLen - 1; i++, pos++)
        buf[pos] = prefixDev[i];
    for (ULONG i = 0; core[i] && pos < maxLen - 1; i++, pos++)
        buf[pos] = core[i];
    buf[pos] = L'\0';
}

NTSTATUS DriverEntry()
{
    ULONG seed = ComputeSharedSeed();

    // Fully randomized pool tag
    g_PoolTag = seed ^ (seed << 8) ^ (seed << 16) ^ (seed << 24);
    if (g_PoolTag == 0) g_PoolTag = 'rwTb';

    g_IoctlBase = IoctlDerivation::DeriveIoctlBase(seed);

    BuildDeviceName(g_DeviceName, 32);

    // Build symlink name: \DosDevices\{core}
    WCHAR dosPrefix[] = {
        L'\\', L'D', L'o', L's', L'D', L'e', L'v', L'i', L'c', L'e', L's', L'\\', 0
    };
    ULONG pos = 0;
    RtlZeroMemory(g_SymlinkName, sizeof(g_SymlinkName));
    for (ULONG i = 0; dosPrefix[i] && pos < 35; i++, pos++)
        g_SymlinkName[pos] = dosPrefix[i];
    for (ULONG i = 8; g_DeviceName[i] && pos < 35; i++, pos++)
        g_SymlinkName[pos] = g_DeviceName[i];
    g_SymlinkName[pos] = L'\0';

    // Allocate fake DRIVER_OBJECT since we were not called by the kernel's loader
    PDRIVER_OBJECT DriverObject = (PDRIVER_OBJECT)ExAllocatePoolWithTag(
        NonPagedPool, sizeof(DRIVER_OBJECT), g_PoolTag);
    if (!DriverObject)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(DriverObject, sizeof(DRIVER_OBJECT));
    DriverObject->Type = IO_TYPE_DRIVER;
    DriverObject->Size = (SHORT)sizeof(DRIVER_OBJECT);

    // Allocate and link minimal DRIVER_EXTENSION
    PDRIVER_EXTENSION Extension = (PDRIVER_EXTENSION)ExAllocatePoolWithTag(
        NonPagedPool, sizeof(DRIVER_EXTENSION), g_PoolTag);
    if (!Extension)
    {
        ExFreePoolWithTag(DriverObject, g_PoolTag);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(Extension, sizeof(DRIVER_EXTENSION));
    Extension->DriverObject = DriverObject;
    DriverObject->DriverExtension = Extension;

    for (ULONG i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        DriverObject->MajorFunction[i] = BitwareCreateClose;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = BitwareCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = BitwareCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = BitwareDeviceControl;
    DriverObject->DriverUnload = NULL;

    UNICODE_STRING deviceName;
    RtlInitUnicodeString(&deviceName, g_DeviceName);

    NTSTATUS status = IoCreateDevice(
        DriverObject, 0, &deviceName,
        FILE_DEVICE_UNKNOWN, 0, FALSE, &g_DeviceObject
    );
    if (!NT_SUCCESS(status))
    {
        ExFreePoolWithTag(Extension, g_PoolTag);
        ExFreePoolWithTag(DriverObject, g_PoolTag);
        return status;
    }

    UNICODE_STRING symLink;
    RtlInitUnicodeString(&symLink, g_SymlinkName);

    status = IoCreateSymbolicLink(&symLink, &deviceName);
    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
        ExFreePoolWithTag(Extension, g_PoolTag);
        ExFreePoolWithTag(DriverObject, g_PoolTag);
        return status;
    }

    g_DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}
