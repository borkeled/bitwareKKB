#include <ntddk.h>
#include "BitwareDrv.h"

typedef struct _BITWARE_SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    LONG BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG_PTR PageDirectoryBase;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
} BITWARE_SYSTEM_PROCESS_INFORMATION, *PBITWARE_SYSTEM_PROCESS_INFORMATION;

#define BITWARE_SYSTEM_INFO_CLASS 5

extern "C"
{
    NTSTATUS ZwQuerySystemInformation(
        ULONG SystemInformationClass,
        PVOID SystemInformation,
        ULONG SystemInformationLength,
        PULONG ReturnLength
    );
}

NTSTATUS BitwareFindProcessByName(PCWSTR ProcessName, PULONG OutProcessId)
{
    NTSTATUS status;
    ULONG bufferSize = 256 * 1024;
    PVOID buffer = ExAllocatePool2(POOL_FLAG_NON_PAGED, bufferSize, 'nBsP');
    if (!buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ULONG returnLength = 0;
    status = ZwQuerySystemInformation(BITWARE_SYSTEM_INFO_CLASS, buffer, bufferSize, &returnLength);
    if (!NT_SUCCESS(status))
    {
        ExFreePool(buffer);
        return status;
    }

    PBITWARE_SYSTEM_PROCESS_INFORMATION spi = (PBITWARE_SYSTEM_PROCESS_INFORMATION)buffer;
    BOOLEAN found = FALSE;

    UNICODE_STRING targetName;
    RtlInitUnicodeString(&targetName, ProcessName);

    while (TRUE)
    {
        if (spi->ImageName.Buffer && spi->ImageName.Length > 0)
        {
            if (RtlCompareUnicodeString(&spi->ImageName, &targetName, TRUE) == 0)
            {
                *OutProcessId = HandleToULong(spi->UniqueProcessId);
                found = TRUE;
                break;
            }
        }

        if (spi->NextEntryOffset == 0)
        {
            break;
        }

        spi = (PBITWARE_SYSTEM_PROCESS_INFORMATION)((PUCHAR)spi + spi->NextEntryOffset);
    }

    ExFreePool(buffer);
    return found ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}
