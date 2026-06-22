#include <ntddk.h>
#include "BitwareDrv.h"

extern "C"
{
    NTSTATUS PsLookupProcessByProcessId(HANDLE ProcessId, PEPROCESS* Process);
    NTSTATUS MmCopyVirtualMemory(
        PEPROCESS SourceProcess,
        PVOID SourceAddress,
        PEPROCESS TargetProcess,
        PVOID TargetAddress,
        SIZE_T BufferSize,
        KPROCESSOR_MODE PreviousMode,
        PSIZE_T ReturnSize
    );
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

    SIZE_T bytesRead = 0;
    status = MmCopyVirtualMemory(
        targetProcess,
        (PVOID)(ULONG_PTR)Address,
        PsGetCurrentProcess(),
        Buffer,
        Size,
        KernelMode,
        &bytesRead
    );

    ObDereferenceObject(targetProcess);
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

    SIZE_T bytesWritten = 0;
    status = MmCopyVirtualMemory(
        PsGetCurrentProcess(),
        Buffer,
        targetProcess,
        (PVOID)(ULONG_PTR)Address,
        Size,
        KernelMode,
        &bytesWritten
    );

    ObDereferenceObject(targetProcess);
    return status;
}
