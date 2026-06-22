#include <ntddk.h>
#include "BitwareDrv.h"

#define DEVICE_NAME  L"\\Device\\BitwareDevice"
#define SYMLINK_NAME L"\\DosDevices\\BitwareDevice"

PDEVICE_OBJECT g_DeviceObject = NULL;

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
    if (g_DeviceObject)
    {
        UNICODE_STRING symLink;
        RtlInitUnicodeString(&symLink, SYMLINK_NAME);
        IoDeleteSymbolicLink(&symLink);
        IoDeleteDevice(g_DeviceObject);
    }
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
    {
        DriverObject->MajorFunction[i] = BitwareCreateClose;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = BitwareCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = BitwareCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = BitwareDeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    UNICODE_STRING deviceName;
    RtlInitUnicodeString(&deviceName, DEVICE_NAME);

    NTSTATUS status = IoCreateDevice(
        DriverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &g_DeviceObject
    );

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    UNICODE_STRING symLink;
    RtlInitUnicodeString(&symLink, SYMLINK_NAME);

    status = IoCreateSymbolicLink(&symLink, &deviceName);
    if (!NT_SUCCESS(status))
    {
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
        return status;
    }

    return STATUS_SUCCESS;
}
