#include <ntddk.h>
#include "BitwareDrv.h"

extern "C"
{
    NTSTATUS PsLookupProcessByProcessId(HANDLE ProcessId, PEPROCESS* Process);
    VOID KeStackAttachProcess(PEPROCESS Process, PBITWARE_KAPC_STATE ApcState);
    VOID KeUnstackDetachProcess(PBITWARE_KAPC_STATE ApcState);
}

NTSTATUS BitwareReadProcessMemory(HANDLE ProcessId, ULONG64 Address, PVOID Buffer, ULONG Size)
{
    if (Size == 0 || Buffer == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    PEPROCESS targetProcess = NULL;
    NTSTATUS status = PsLookupProcessByProcessId(ProcessId, &targetProcess);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    PVOID kernelBuffer = ExAllocatePoolWithTag(NonPagedPool, Size, g_PoolTag);
    if (kernelBuffer == NULL)
    {
        ObDereferenceObject(targetProcess);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    BITWARE_KAPC_STATE apcState;
    KeStackAttachProcess(targetProcess, &apcState);

    __try
    {
        ProbeForRead((PVOID)(ULONG_PTR)Address, Size, 1);
        RtlCopyMemory(kernelBuffer, (PVOID)(ULONG_PTR)Address, Size);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    KeUnstackDetachProcess(&apcState);
    ObDereferenceObject(targetProcess);

    if (NT_SUCCESS(status))
    {
        __try
        {
            ProbeForWrite(Buffer, Size, 1);
            RtlCopyMemory(Buffer, kernelBuffer, Size);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }
    }

    ExFreePoolWithTag(kernelBuffer, g_PoolTag);
    return status;
}

NTSTATUS BitwareWriteProcessMemory(HANDLE ProcessId, ULONG64 Address, PVOID Buffer, ULONG Size)
{
    if (Size == 0 || Buffer == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    PEPROCESS targetProcess = NULL;
    NTSTATUS status = PsLookupProcessByProcessId(ProcessId, &targetProcess);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    PVOID kernelBuffer = ExAllocatePoolWithTag(NonPagedPool, Size, g_PoolTag);
    if (kernelBuffer == NULL)
    {
        ObDereferenceObject(targetProcess);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Securely capture the caller's buffer data before context switching
    __try
    {
        ProbeForRead(Buffer, Size, 1);
        RtlCopyMemory(kernelBuffer, Buffer, Size);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    if (NT_SUCCESS(status))
    {
        BITWARE_KAPC_STATE apcState;
        KeStackAttachProcess(targetProcess, &apcState);

        __try
        {
            ProbeForWrite((PVOID)(ULONG_PTR)Address, Size, 1);
            RtlCopyMemory((PVOID)(ULONG_PTR)Address, kernelBuffer, Size);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }

        KeUnstackDetachProcess(&apcState);
    }

    ObDereferenceObject(targetProcess);
    ExFreePoolWithTag(kernelBuffer, g_PoolTag);
    return status;
}
