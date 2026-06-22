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




