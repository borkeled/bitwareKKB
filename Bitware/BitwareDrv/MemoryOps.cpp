#include <ntddk.h>
#include "BitwareDrv.h"

typedef struct _BITWARE_KAPC_STATE {
    LIST_ENTRY ApcListHead[2];
    PEPROCESS Process;
    BOOLEAN KernelApcInProgress;
    BOOLEAN KernelApcPending;
    BOOLEAN UserApcPending;
    UCHAR ApcStateIndex;
} BITWARE_KAPC_STATE;

typedef BITWARE_KAPC_STATE* PBITWARE_KAPC_STATE;

extern "C"
{
    NTSTATUS PsLookupProcessByProcessId(HANDLE ProcessId, PEPROCESS* Process);
    VOID KeStackAttachProcess(PEPROCESS Process, PBITWARE_KAPC_STATE ApcState);
    VOID KeUnstackDetachProcess(PBITWARE_KAPC_STATE ApcState);
}

NTSTATUS BitwareReadProcessMemory(HANDLE ProcessHandle, ULONG64 Address, PVOID Buffer, ULONG Size)
{
    PEPROCESS targetProcess = NULL;
    NTSTATUS status = PsLookupProcessByProcessId(ProcessHandle, &targetProcess);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    BITWARE_KAPC_STATE apcState;
    KeStackAttachProcess(targetProcess, &apcState);

    __try
    {
        ProbeForWrite(Buffer, Size, 1);
        RtlCopyMemory(Buffer, (PVOID)(ULONG_PTR)Address, Size);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    KeUnstackDetachProcess(&apcState);
    ObDereferenceObject(targetProcess);

    return status;
}

NTSTATUS BitwareWriteProcessMemory(HANDLE ProcessHandle, ULONG64 Address, PVOID Buffer, ULONG Size)
{
    PEPROCESS targetProcess = NULL;
    NTSTATUS status = PsLookupProcessByProcessId(ProcessHandle, &targetProcess);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    BITWARE_KAPC_STATE apcState;
    KeStackAttachProcess(targetProcess, &apcState);

    __try
    {
        ProbeForRead(Buffer, Size, 1);
        ProbeForWrite((PVOID)(ULONG_PTR)Address, Size, 1);
        RtlCopyMemory((PVOID)(ULONG_PTR)Address, Buffer, Size);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    KeUnstackDetachProcess(&apcState);
    ObDereferenceObject(targetProcess);

    return status;
}
