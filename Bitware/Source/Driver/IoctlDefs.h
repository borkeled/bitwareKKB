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

#define BITWARE_IOCTL_READ_MEMORY  0
#define BITWARE_IOCTL_WRITE_MEMORY 1
#define BITWARE_IOCTL_FIND_PROCESS 2
#define BITWARE_IOCTL_FIND_MODULE  3
#define BITWARE_IOCTL_UNLOAD       4
#define BITWARE_IOCTL_READ_INPUT   5
#define BITWARE_IOCTL_COUNT        6

constexpr ULONG kBitwareSeed = 0xA3F7C92E;

namespace IoctlDerivation {

    inline void DeriveDeviceName(unsigned long seed, wchar_t* name, unsigned long maxLen)
    {
        if (maxLen < 5) return;
        unsigned long s = seed;
        name[0] = L'A' + (s % 26); s >>= 5;
        name[1] = L'a' + (s % 26); s >>= 5;
        name[2] = L'A' + (s % 26); s >>= 5;
        name[3] = L'a' + (s % 26); s >>= 5;
        name[4] = L'\0';
    }

    inline unsigned long DeriveIoctlBase(unsigned long seed)
    {
        return ((seed >> 4) & 0x1FFFFF) << 2;
    }

}

#pragma pack(push, 1)

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
