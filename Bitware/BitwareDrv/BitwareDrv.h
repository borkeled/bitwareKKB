#pragma once
#include <ntddk.h>

#include <IoctlDefs.h>

// Forged pool tags mimicking common Windows tag patterns
#define POOL_TAG_MEMORY 'IoNm'   // "mN oI" — I/O Notification
#define POOL_TAG_PROCESS 'MmSt'  // "tS mM" — Memory State
#define POOL_TAG_MODULE 'Tp01'   // "10 pT" — Thread Pool

extern ULONG g_PoolTag;
extern ULONG g_IoctlBase;
extern WCHAR g_DeviceName[];
extern WCHAR g_SymlinkName[];

typedef struct _BITWARE_KAPC_STATE {
    LIST_ENTRY ApcListHead[2];
    PEPROCESS Process;
    BOOLEAN KernelApcInProgress;
    BOOLEAN KernelApcPending;
    BOOLEAN UserApcPending;
    UCHAR ApcStateIndex;
} BITWARE_KAPC_STATE;

typedef BITWARE_KAPC_STATE* PBITWARE_KAPC_STATE;

// Keyboard input capture
NTSTATUS InitKeyboardCapture();
VOID CleanupKeyboardCapture();
ULONG KeyboardRingBufferRead(PBITWARE_KEYBOARD_DATA out, ULONG maxCount);


