#pragma once

#ifndef CTL_CODE
#define CTL_CODE(DeviceType, Function, Method, Access) \
    (((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method))
#endif

#ifndef FILE_DEVICE_UNKNOWN
#define FILE_DEVICE_UNKNOWN 0x00000022
#endif

#ifndef METHOD_BUFFERED
#define METHOD_BUFFERED 0
#endif

#ifndef FILE_ANY_ACCESS
#define FILE_ANY_ACCESS 0
#endif

#ifndef FILE_READ_ACCESS
#define FILE_READ_ACCESS 0x0001
#endif

#ifndef FILE_WRITE_ACCESS
#define FILE_WRITE_ACCESS 0x0002
#endif

#define IOCTL_BITWARE_READ_MEMORY \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_BITWARE_WRITE_MEMORY \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_BITWARE_FIND_PROCESS \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_BITWARE_FIND_MODULE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_BITWARE_UNLOAD \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_BITWARE_READ_INPUT \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_BITWARE_BOOTSTRAP \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x700, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Runtime IOCTL indices (added to g_IoctlBase at runtime)
#define BITWARE_IOCTL_READ_MEMORY  0
#define BITWARE_IOCTL_WRITE_MEMORY 1
#define BITWARE_IOCTL_FIND_PROCESS 2
#define BITWARE_IOCTL_FIND_MODULE  3
#define BITWARE_IOCTL_UNLOAD       4
#define BITWARE_IOCTL_READ_INPUT   5

#pragma pack(push, 1)

typedef struct _BITWARE_BOOTSTRAP_RESPONSE {
    unsigned long IoctlBase;
    unsigned long IoctlReadMemory;
    unsigned long IoctlWriteMemory;
    unsigned long IoctlFindProcess;
    unsigned long IoctlFindModule;
    unsigned long IoctlUnload;
    unsigned long IoctlReadInput;
} BITWARE_BOOTSTRAP_RESPONSE, *PBITWARE_BOOTSTRAP_RESPONSE;

typedef struct _BITWARE_KEYBOARD_DATA {
    unsigned short MakeCode;
    unsigned short Flags;
    unsigned short UnitId;
} BITWARE_KEYBOARD_DATA, *PBITWARE_KEYBOARD_DATA;

typedef struct _BITWARE_READ_INPUT {
    unsigned long ProcessId;
    unsigned long long Address;
    unsigned long Size;
} BITWARE_READ_INPUT, *PBITWARE_READ_INPUT;

typedef struct _BITWARE_WRITE_INPUT {
    unsigned long ProcessId;
    unsigned long long Address;
    unsigned long Size;
} BITWARE_WRITE_INPUT, *PBITWARE_WRITE_INPUT;

typedef struct _BITWARE_FIND_PROCESS_INPUT {
    wchar_t Name[260];
} BITWARE_FIND_PROCESS_INPUT, *PBITWARE_FIND_PROCESS_INPUT;

typedef struct _BITWARE_FIND_MODULE_INPUT {
    unsigned long ProcessId;
    wchar_t Name[260];
} BITWARE_FIND_MODULE_INPUT, *PBITWARE_FIND_MODULE_INPUT;

#pragma pack(pop)
