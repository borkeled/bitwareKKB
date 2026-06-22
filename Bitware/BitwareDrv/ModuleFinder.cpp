#include <ntddk.h>
#include "BitwareDrv.h"

// Undocumented Kernel Exports
extern "C"
{
    NTSTATUS PsLookupProcessByProcessId(HANDLE ProcessId, PEPROCESS* Process);
    PVOID NTAPI PsGetProcessPeb(PEPROCESS Process);
    PVOID NTAPI PsGetProcessWow64Process(PEPROCESS Process);
    NTSTATUS MmCopyVirtualMemory(PEPROCESS SourceProcess, PVOID SourceAddress, PEPROCESS TargetProcess, PVOID TargetAddress, SIZE_T BufferSize, KPROCESSOR_MODE PreviousMode, PSIZE_T ReturnSize);
}

// Native 64-Bit PEB & Loader Structures (PEB is incomplete in public headers)
typedef struct _BITWARE_PEB_LDR_DATA {
    ULONG Length;
    BOOLEAN Initialized;
    HANDLE SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
} BITWARE_PEB_LDR_DATA, *PBITWARE_PEB_LDR_DATA;

typedef struct _BITWARE_PEB {
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    UCHAR BitField;
    HANDLE Mutant;
    PVOID ImageBaseAddress;
    PBITWARE_PEB_LDR_DATA Ldr;
} BITWARE_PEB, *PBITWARE_PEB;

typedef struct _BITWARE_LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
} BITWARE_LDR_DATA_TABLE_ENTRY, *PBITWARE_LDR_DATA_TABLE_ENTRY;

// Wow64 (32-bit) PEB & Loader Structures (uses UNICODE_STRING32 from ntdef.h)
typedef struct _PEB_LDR_DATA32 {
    ULONG Length;
    BOOLEAN Initialized;
    ULONG SsHandle;
    LIST_ENTRY32 InLoadOrderModuleList;
} PEB_LDR_DATA32, *PPEB_LDR_DATA32;

typedef struct _PEB32 {
    UCHAR InheritedAddressSpace;
    UCHAR ReadImageFileExecOptions;
    UCHAR BeingDebugged;
    UCHAR BitField;
    ULONG Mutant;
    ULONG ImageBaseAddress;
    ULONG Ldr;
} PEB32, *PPEB32;

typedef struct _LDR_DATA_TABLE_ENTRY32 {
    LIST_ENTRY32 InLoadOrderLinks;
    LIST_ENTRY32 InMemoryOrderLinks;
    LIST_ENTRY32 InInitializationOrderLinks;
    ULONG DllBase;
    ULONG EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING32 FullDllName;
    UNICODE_STRING32 BaseDllName;
} LDR_DATA_TABLE_ENTRY32, *PLDR_DATA_TABLE_ENTRY32;

static NTSTATUS ReadProcessMemory(PEPROCESS target, PVOID src, PVOID dst, SIZE_T size)
{
    SIZE_T readSize = 0;
    return MmCopyVirtualMemory(target, src, PsGetCurrentProcess(), dst, size, KernelMode, &readSize);
}

static BOOLEAN SafeEqualUnicodeString(PCWSTR string1, PUNICODE_STRING string2, PEPROCESS target)
{
    if (string1 == NULL || string2 == NULL || string2->Buffer == NULL)
        return FALSE;

    USHORT len1 = (USHORT)wcslen(string1);
    if (len1 * sizeof(WCHAR) != string2->Length)
        return FALSE;

    WCHAR* buf = (WCHAR*)ExAllocatePoolWithTag(NonPagedPool, string2->Length, 'mNtS');
    if (buf == NULL)
        return FALSE;

    NTSTATUS status = ReadProcessMemory(target, string2->Buffer, buf, string2->Length);
    if (!NT_SUCCESS(status))
    {
        ExFreePoolWithTag(buf, 'mNtS');
        return FALSE;
    }

    BOOLEAN match = TRUE;
    for (USHORT i = 0; i < len1; i++)
    {
        WCHAR c1 = string1[i];
        WCHAR c2 = buf[i];
        if (c1 >= L'a' && c1 <= L'z') c1 -= 32;
        if (c2 >= L'a' && c2 <= L'z') c2 -= 32;
        if (c1 != c2)
        {
            match = FALSE;
            break;
        }
    }

    ExFreePoolWithTag(buf, 'mNtS');
    return match;
}

static BOOLEAN SafeEqualUnicodeString32(PCWSTR string1, PUNICODE_STRING32 string2, PEPROCESS target)
{
    if (string1 == NULL || string2 == NULL || string2->Buffer == 0)
        return FALSE;

    USHORT len1 = (USHORT)wcslen(string1);
    if (len1 * sizeof(WCHAR) != string2->Length)
        return FALSE;

    WCHAR* buf = (WCHAR*)ExAllocatePoolWithTag(NonPagedPool, string2->Length, 'mNtS');
    if (buf == NULL)
        return FALSE;

    NTSTATUS status = ReadProcessMemory(target, (PVOID)(ULONG_PTR)string2->Buffer, buf, string2->Length);
    if (!NT_SUCCESS(status))
    {
        ExFreePoolWithTag(buf, 'mNtS');
        return FALSE;
    }

    BOOLEAN match = TRUE;
    for (USHORT i = 0; i < len1; i++)
    {
        WCHAR c1 = string1[i];
        WCHAR c2 = buf[i];
        if (c1 >= L'a' && c1 <= L'z') c1 -= 32;
        if (c2 >= L'a' && c2 <= L'z') c2 -= 32;
        if (c1 != c2)
        {
            match = FALSE;
            break;
        }
    }

    ExFreePoolWithTag(buf, 'mNtS');
    return match;
}

NTSTATUS BitwareFindModuleByName(ULONG ProcessId, PCWSTR ModuleName, PULONG64 OutBaseAddress)
{
    if (ModuleName == NULL || OutBaseAddress == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    PEPROCESS targetProcess = NULL;
    NTSTATUS status = PsLookupProcessByProcessId(ULongToHandle(ProcessId), &targetProcess);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    BOOLEAN found = FALSE;
    ULONG64 baseAddress = 0;

    PVOID wow64Process = PsGetProcessWow64Process(targetProcess);

    if (wow64Process != NULL)
    {
        // Walk Wow64 (32-bit) PEB using MmCopyVirtualMemory
        PEB32 peb32;
        status = ReadProcessMemory(targetProcess, wow64Process, &peb32, sizeof(peb32));
        if (!NT_SUCCESS(status))
        {
            ObDereferenceObject(targetProcess);
            return status;
        }

        if (peb32.Ldr != 0)
        {
            PEB_LDR_DATA32 ldr32;
            status = ReadProcessMemory(targetProcess, (PVOID)(ULONG_PTR)peb32.Ldr, &ldr32, sizeof(ldr32));
            if (!NT_SUCCESS(status))
            {
                ObDereferenceObject(targetProcess);
                return status;
            }

            ULONG head = ldr32.InLoadOrderModuleList.Flink;
            ULONG curr = head;

            while (curr != 0 && curr != peb32.Ldr + FIELD_OFFSET(PEB_LDR_DATA32, InLoadOrderModuleList))
            {
                LDR_DATA_TABLE_ENTRY32 entry;
                status = ReadProcessMemory(targetProcess, (PVOID)(ULONG_PTR)curr, &entry, sizeof(entry));
                if (!NT_SUCCESS(status))
                    break;

                if (SafeEqualUnicodeString32(ModuleName, &entry.BaseDllName, targetProcess))
                {
                    baseAddress = entry.DllBase;
                    found = TRUE;
                    break;
                }

                curr = entry.InLoadOrderLinks.Flink;
            }
        }
    }
    else
    {
        // Walk native 64-bit PEB using MmCopyVirtualMemory
        PVOID pebAddr = PsGetProcessPeb(targetProcess);
        if (pebAddr != NULL)
        {
            BITWARE_PEB peb;
            status = ReadProcessMemory(targetProcess, pebAddr, &peb, sizeof(peb));
            if (!NT_SUCCESS(status))
            {
                ObDereferenceObject(targetProcess);
                return status;
            }

            if (peb.Ldr != NULL)
            {
                BITWARE_PEB_LDR_DATA ldr;
                status = ReadProcessMemory(targetProcess, peb.Ldr, &ldr, sizeof(ldr));
                if (!NT_SUCCESS(status))
                {
                    ObDereferenceObject(targetProcess);
                    return status;
                }

                LIST_ENTRY head = ldr.InLoadOrderModuleList;
                LIST_ENTRY curr = {0};
                status = ReadProcessMemory(targetProcess, head.Flink, &curr, sizeof(curr));
                if (!NT_SUCCESS(status))
                {
                    ObDereferenceObject(targetProcess);
                    return status;
                }

                while (curr.Flink != NULL && curr.Flink != head.Flink)
                {
                    BITWARE_LDR_DATA_TABLE_ENTRY entry;
                    status = ReadProcessMemory(targetProcess, CONTAINING_RECORD(&curr, BITWARE_LDR_DATA_TABLE_ENTRY, InLoadOrderLinks), &entry, sizeof(entry));
                    if (!NT_SUCCESS(status))
                        break;

                    if (SafeEqualUnicodeString(ModuleName, &entry.BaseDllName, targetProcess))
                    {
                        baseAddress = (ULONG64)entry.DllBase;
                        found = TRUE;
                        break;
                    }

                    LIST_ENTRY next = {0};
                    status = ReadProcessMemory(targetProcess, entry.InLoadOrderLinks.Flink, &next, sizeof(next));
                    if (!NT_SUCCESS(status))
                        break;

                    curr = next;
                }
            }
        }
    }

    ObDereferenceObject(targetProcess);

    if (found)
    {
        *OutBaseAddress = baseAddress;
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_FOUND;
}
