#include <ntddk.h>
#include "BitwareDrv.h"

extern "C"
{
    NTSTATUS PsLookupProcessByProcessId(HANDLE ProcessId, PEPROCESS* Process);
    PVOID PsGetProcessSectionBaseAddress(PEPROCESS Process);
}

NTSTATUS BitwareFindModuleByName(ULONG ProcessId, PCWSTR ModuleName, PULONG64 OutBaseAddress)
{
    PEPROCESS targetProcess = NULL;
    NTSTATUS status = PsLookupProcessByProcessId(ULongToHandle(ProcessId), &targetProcess);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    PVOID sectionBase = PsGetProcessSectionBaseAddress(targetProcess);

    ObDereferenceObject(targetProcess);

    if (sectionBase)
    {
        *OutBaseAddress = (ULONG64)sectionBase;
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}
