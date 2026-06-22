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
    if (ProcessName == NULL || OutProcessId == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status;
    ULONG bufferSize = 256 * 1024; // Start with 256 KB
    PVOID buffer = NULL;

    // Loop to dynamically size the buffer in case of length mismatches
    do
    {
        buffer = ExAllocatePoolWithTag(PagedPool, bufferSize, 'nBsP');
        if (!buffer)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ULONG returnLength = 0;
        status = ZwQuerySystemInformation(BITWARE_SYSTEM_INFO_CLASS, buffer, bufferSize, &returnLength);

        if (status == STATUS_INFO_LENGTH_MISMATCH)
        {
            ExFreePoolWithTag(buffer, 'nBsP');
            buffer = NULL;
            
            // Add a safety padding margin to account for new processes spawning between calls
            bufferSize = returnLength + (32 * 1024);
        }
        else if (!NT_SUCCESS(status))
        {
            ExFreePoolWithTag(buffer, 'nBsP');
            return status;
        }
    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    PBITWARE_SYSTEM_PROCESS_INFORMATION spi = (PBITWARE_SYSTEM_PROCESS_INFORMATION)buffer;
    PUCHAR bufferEnd = (PUCHAR)buffer + bufferSize;
    BOOLEAN found = FALSE;

    UNICODE_STRING targetName;
    RtlInitUnicodeString(&targetName, ProcessName);

    while (TRUE)
    {
        // Boundary sanity check to prevent out-of-bounds reading
        if ((PUCHAR)spi + sizeof(BITWARE_SYSTEM_PROCESS_INFORMATION) > bufferEnd)
        {
            break;
        }

        if (spi->ImageName.Buffer && spi->ImageName.Length > 0)
        {
            // Verify that the string buffer resides fully inside our allocated pool limits
            if ((PUCHAR)spi->ImageName.Buffer >= (PUCHAR)buffer && 
                ((PUCHAR)spi->ImageName.Buffer + spi->ImageName.Length) <= bufferEnd)
            {
                if (RtlCompareUnicodeString(&spi->ImageName, &targetName, TRUE) == 0)
                {
                    *OutProcessId = HandleToULong(spi->UniqueProcessId);
                    found = TRUE;
                    break;
                }
            }
        }

        if (spi->NextEntryOffset == 0)
        {
            break;
        }

        // Advance and verify that the next entry's offset is within buffer limits
        ULONG_PTR nextAddress = (ULONG_PTR)spi + spi->NextEntryOffset;
        if (nextAddress >= (ULONG_PTR)bufferEnd || nextAddress < (ULONG_PTR)buffer)
        {
            break;
        }

        spi = (PBITWARE_SYSTEM_PROCESS_INFORMATION)nextAddress;
    }

    ExFreePoolWithTag(buffer, 'nBsP');
    return found ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}
