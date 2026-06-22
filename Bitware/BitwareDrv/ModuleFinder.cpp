#include <ntddk.h>
#include "BitwareDrv.h"

// Undocumented Kernel Exports
extern "C"
{
    NTSTATUS PsLookupProcessByProcessId(HANDLE ProcessId, PEPROCESS* Process);
    PVOID NTAPI PsGetProcessPeb(PEPROCESS Process);
    PVOID NTAPI PsGetProcessWow64Process(PEPROCESS Process);
    VOID KeStackAttachProcess(PEPROCESS Process, PBITWARE_KAPC_STATE ApcState);
    VOID KeUnstackDetachProcess(PBITWARE_KAPC_STATE ApcState);
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

// Safe comparison helper for native structures
static BOOLEAN SafeEqualUnicodeString(PCWSTR string1, PUNICODE_STRING string2)
{
    if (string1 == NULL || string2 == NULL || string2->Buffer == NULL)
        return FALSE;

    USHORT len1 = (USHORT)wcslen(string1);
    if (len1 * sizeof(WCHAR) != string2->Length)
        return FALSE;

    __try
    {
        for (USHORT i = 0; i < len1; i++)
        {
            WCHAR c1 = string1[i];
            WCHAR c2 = string2->Buffer[i];

            if (c1 >= L'a' && c1 <= L'z') c1 -= 32;
            if (c2 >= L'a' && c2 <= L'z') c2 -= 32;

            if (c1 != c2)
                return FALSE;
        }
        return TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }
}

// Safe comparison helper for Wow64 structures
static BOOLEAN SafeEqualUnicodeString32(PCWSTR string1, PUNICODE_STRING32 string2)
{
    if (string1 == NULL || string2 == NULL || string2->Buffer == 0)
        return FALSE;

    USHORT len1 = (USHORT)wcslen(string1);
    if (len1 * sizeof(WCHAR) != string2->Length)
        return FALSE;

    PWCHAR buffer = (PWCHAR)(ULONG_PTR)string2->Buffer;

    __try
    {
        for (USHORT i = 0; i < len1; i++)
        {
            WCHAR c1 = string1[i];
            WCHAR c2 = buffer[i];

            if (c1 >= L'a' && c1 <= L'z') c1 -= 32;
            if (c2 >= L'a' && c2 <= L'z') c2 -= 32;

            if (c1 != c2)
                return FALSE;
        }
        return TRUE;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }
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

    BITWARE_KAPC_STATE apcState;
    KeStackAttachProcess(targetProcess, &apcState);

    BOOLEAN found = FALSE;
    ULONG64 baseAddress = 0;

    __try
    {
        PVOID wow64Process = PsGetProcessWow64Process(targetProcess);
        if (wow64Process != NULL)
        {
            // Walk Wow64 (32-bit) PEB
            PPEB32 peb32 = (PPEB32)wow64Process;
            if (peb32->Ldr != 0)
            {
                PPEB_LDR_DATA32 ldr32 = (PPEB_LDR_DATA32)(ULONG_PTR)peb32->Ldr;
                ULONG head = ldr32->InLoadOrderModuleList.Flink;
                ULONG curr = head;

                while (curr != 0 && curr != (ULONG)(ULONG_PTR)&ldr32->InLoadOrderModuleList)
                {
                    PLDR_DATA_TABLE_ENTRY32 entry = (PLDR_DATA_TABLE_ENTRY32)(ULONG_PTR)curr;

                    if (SafeEqualUnicodeString32(ModuleName, &entry->BaseDllName))
                    {
                        baseAddress = entry->DllBase;
                        found = TRUE;
                        break;
                    }

                    curr = entry->InLoadOrderLinks.Flink;
                }
            }
        }
        else
        {
            // Walk native 64-bit PEB
            PBITWARE_PEB peb = (PBITWARE_PEB)PsGetProcessPeb(targetProcess);
            if (peb != NULL && peb->Ldr != NULL)
            {
                PBITWARE_PEB_LDR_DATA ldr = peb->Ldr;
                PLIST_ENTRY head = &ldr->InLoadOrderModuleList;
                PLIST_ENTRY curr = head->Flink;

                while (curr != NULL && curr != head)
                {
                    PBITWARE_LDR_DATA_TABLE_ENTRY entry = CONTAINING_RECORD(curr, BITWARE_LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

                    if (SafeEqualUnicodeString(ModuleName, &entry->BaseDllName))
                    {
                        baseAddress = (ULONG64)entry->DllBase;
                        found = TRUE;
                        break;
                    }

                    curr = curr->Flink;
                }
            }
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    KeUnstackDetachProcess(&apcState);
    ObDereferenceObject(targetProcess);

    if (NT_SUCCESS(status))
    {
        if (found)
        {
            *OutBaseAddress = baseAddress;
            return STATUS_SUCCESS;
        }
        else
        {
            return STATUS_NOT_FOUND;
        }
    }

    return status;
}
