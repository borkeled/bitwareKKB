#pragma once

#define BITWARE_DEVICE_NAME  L"\\Device\\BitwareDevice"
#define BITWARE_SYMLINK_NAME L"\\DosDevices\\BitwareDevice"
#define BITWARE_WIN32_NAME   L"\\\\.\\BitwareDevice"

#include <IoctlDefs.h>

typedef struct _BITWARE_KAPC_STATE {
    LIST_ENTRY ApcListHead[2];
    PEPROCESS Process;
    BOOLEAN KernelApcInProgress;
    BOOLEAN KernelApcPending;
    BOOLEAN UserApcPending;
    UCHAR ApcStateIndex;
} BITWARE_KAPC_STATE;

typedef BITWARE_KAPC_STATE* PBITWARE_KAPC_STATE;
