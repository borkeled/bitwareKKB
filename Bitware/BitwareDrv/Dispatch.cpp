#include <ntddk.h>
#include "BitwareDrv.h"

extern PDEVICE_OBJECT g_DeviceObject;

NTSTATUS BitwareReadProcessMemory(HANDLE ProcessHandle, ULONG64 Address, PVOID Buffer, ULONG Size);
NTSTATUS BitwareWriteProcessMemory(HANDLE ProcessHandle, ULONG64 Address, PVOID Buffer, ULONG Size);
NTSTATUS BitwareFindProcessByName(PCWSTR ProcessName, PULONG OutProcessId);
NTSTATUS BitwareFindModuleByName(ULONG ProcessId, PCWSTR ModuleName, PULONG64 OutBaseAddress);

NTSTATUS BitwareDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG ioctlCode = ioStack->Parameters.DeviceIoControl.IoControlCode;
    ULONG inLen = ioStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLen = ioStack->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID sysBuf = Irp->AssociatedIrp.SystemBuffer;
    ULONG_PTR info = 0;

    if (sysBuf == NULL)
    {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    switch (ioctlCode)
    {
        case IOCTL_BITWARE_READ_MEMORY:
        {
            if (inLen < sizeof(BITWARE_READ_INPUT) || outLen == 0)
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PBITWARE_READ_INPUT input = (PBITWARE_READ_INPUT)sysBuf;
            ULONG readSize = (input->Size < outLen) ? input->Size : outLen;

            status = BitwareReadProcessMemory(
                ULongToHandle(input->ProcessId),
                input->Address,
                sysBuf,
                readSize
            );

            if (NT_SUCCESS(status))
            {
                info = readSize;
            }
            break;
        }

        case IOCTL_BITWARE_WRITE_MEMORY:
        {
            if (inLen < sizeof(BITWARE_WRITE_INPUT))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PBITWARE_WRITE_INPUT input = (PBITWARE_WRITE_INPUT)sysBuf;
            ULONG dataSize = input->Size;

            if (inLen < sizeof(BITWARE_WRITE_INPUT) || inLen - sizeof(BITWARE_WRITE_INPUT) < dataSize)
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PVOID dataBuf = (PUCHAR)sysBuf + sizeof(BITWARE_WRITE_INPUT);

            status = BitwareWriteProcessMemory(
                ULongToHandle(input->ProcessId),
                input->Address,
                dataBuf,
                dataSize
            );
            break;
        }

        case IOCTL_BITWARE_FIND_PROCESS:
        {
            if (inLen < sizeof(BITWARE_FIND_PROCESS_INPUT) || outLen < sizeof(ULONG))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PBITWARE_FIND_PROCESS_INPUT input = (PBITWARE_FIND_PROCESS_INPUT)sysBuf;
            
            ULONG offset = FIELD_OFFSET(BITWARE_FIND_PROCESS_INPUT, Name);
            if (inLen > offset)
            {
                ULONG maxChars = (inLen - offset) / sizeof(WCHAR);
                if (maxChars > 0)
                {
                    input->Name[maxChars - 1] = L'\0';
                }
            }

            ULONG pid = 0;
            status = BitwareFindProcessByName(input->Name, &pid);
            if (NT_SUCCESS(status))
            {
                *(PULONG)sysBuf = pid;
                info = sizeof(ULONG);
            }
            break;
        }

        case IOCTL_BITWARE_FIND_MODULE:
        {
            if (inLen < sizeof(BITWARE_FIND_MODULE_INPUT) || outLen < sizeof(ULONG64))
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PBITWARE_FIND_MODULE_INPUT input = (PBITWARE_FIND_MODULE_INPUT)sysBuf;
            
            ULONG offset = FIELD_OFFSET(BITWARE_FIND_MODULE_INPUT, Name);
            if (inLen > offset)
            {
                ULONG maxChars = (inLen - offset) / sizeof(WCHAR);
                if (maxChars > 0)
                {
                    input->Name[maxChars - 1] = L'\0';
                }
            }

            ULONG64 base = 0;
            status = BitwareFindModuleByName(input->ProcessId, input->Name, &base);
            if (NT_SUCCESS(status))
            {
                *(PULONG64)sysBuf = base;
                info = sizeof(ULONG64);
            }
            break;
        }

        case IOCTL_BITWARE_UNLOAD:
        {
            UNICODE_STRING symLink;
            RtlInitUnicodeString(&symLink, L"\\DosDevices\\BitwareDevice");
            IoDeleteSymbolicLink(&symLink);

            if (g_DeviceObject)
            {
                IoDeleteDevice(g_DeviceObject);
                g_DeviceObject = NULL;
            }

            status = STATUS_SUCCESS;
            break;
        }

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}
