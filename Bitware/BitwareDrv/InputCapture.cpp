#include <ntddk.h>
#include <ntddkbd.h>
#include "BitwareDrv.h"

#define INPUT_RING_SIZE 128
#define KEYBOARD_DEVICE_PATH L"\\Device\\KeyboardClass0"

typedef struct _INPUT_RING_BUFFER {
    KEYBOARD_INPUT_DATA Entries[INPUT_RING_SIZE];
    volatile LONG Head;
    volatile LONG Tail;
} INPUT_RING_BUFFER;

static INPUT_RING_BUFFER g_InputRing = {0};
static HANDLE g_KbdHandle = NULL;
static HANDLE g_KbdThreadHandle = NULL;
static volatile BOOLEAN g_KbdCaptureRunning = FALSE;
static KSPIN_LOCK g_RingLock;

static VOID RingBufferPut(PKEYBOARD_INPUT_DATA data)
{
    KIRQL oldIrql;
    KeAcquireSpinLock(&g_RingLock, &oldIrql);

    LONG head = g_InputRing.Head;
    LONG nextHead = (head + 1) % INPUT_RING_SIZE;

    if (nextHead == g_InputRing.Tail)
    {
        g_InputRing.Tail = (g_InputRing.Tail + 1) % INPUT_RING_SIZE;
    }

    g_InputRing.Entries[head] = *data;
    g_InputRing.Head = nextHead;

    KeReleaseSpinLock(&g_RingLock, oldIrql);
}

static ULONG RingBufferRead(PKEYBOARD_INPUT_DATA out, ULONG maxCount)
{
    KIRQL oldIrql;
    KeAcquireSpinLock(&g_RingLock, &oldIrql);

    ULONG count = 0;
    while (g_InputRing.Tail != g_InputRing.Head && count < maxCount)
    {
        out[count] = g_InputRing.Entries[g_InputRing.Tail];
        count++;
        g_InputRing.Tail = (g_InputRing.Tail + 1) % INPUT_RING_SIZE;
    }

    KeReleaseSpinLock(&g_RingLock, oldIrql);
    return count;
}

static VOID KeyboardCaptureThread(PVOID Context)
{
    UNREFERENCED_PARAMETER(Context);
    IO_STATUS_BLOCK iosb;
    KEYBOARD_INPUT_DATA buffer[16];

    while (g_KbdCaptureRunning)
    {
        NTSTATUS status = ZwReadFile(
            g_KbdHandle,
            NULL, NULL, NULL, &iosb,
            buffer, sizeof(buffer),
            NULL, NULL
        );

        if (NT_SUCCESS(status) && iosb.Information > 0)
        {
            ULONG numKeys = (ULONG)(iosb.Information / sizeof(KEYBOARD_INPUT_DATA));
            for (ULONG i = 0; i < numKeys; i++)
            {
                RingBufferPut(&buffer[i]);
            }
        }
        else if (!NT_SUCCESS(status))
        {
            LARGE_INTEGER delay;
            delay.QuadPart = -10000;
            KeDelayExecutionThread(KernelMode, FALSE, &delay);
        }
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS InitKeyboardCapture()
{
    KeInitializeSpinLock(&g_RingLock);

    UNICODE_STRING devName;
    RtlInitUnicodeString(&devName, KEYBOARD_DEVICE_PATH);

    OBJECT_ATTRIBUTES objAttr;
    InitializeObjectAttributes(&objAttr, &devName, OBJ_KERNEL_HANDLE, NULL, NULL);

    IO_STATUS_BLOCK iosb;
    NTSTATUS status = ZwCreateFile(
        &g_KbdHandle,
        GENERIC_READ,
        &objAttr, &iosb, NULL,
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        0, NULL, 0
    );

    if (!NT_SUCCESS(status))
    {
        g_KbdHandle = NULL;
        return status;
    }

    g_KbdCaptureRunning = TRUE;

    HANDLE threadHandle;
    status = PsCreateSystemThread(
        &threadHandle,
        THREAD_ALL_ACCESS,
        NULL, NULL, NULL,
        KeyboardCaptureThread,
        NULL
    );

    if (!NT_SUCCESS(status))
    {
        g_KbdCaptureRunning = FALSE;
        ZwClose(g_KbdHandle);
        g_KbdHandle = NULL;
        return status;
    }

    ZwClose(threadHandle);
    return STATUS_SUCCESS;
}

VOID CleanupKeyboardCapture()
{
    g_KbdCaptureRunning = FALSE;

    if (g_KbdHandle)
    {
        ZwClose(g_KbdHandle);
        g_KbdHandle = NULL;
    }
}

ULONG KeyboardRingBufferRead(PBITWARE_KEYBOARD_DATA out, ULONG maxCount)
{
    KEYBOARD_INPUT_DATA raw[INPUT_RING_SIZE];
    ULONG readMax = (maxCount < INPUT_RING_SIZE) ? maxCount : INPUT_RING_SIZE;
    ULONG count = RingBufferRead(raw, readMax);

    for (ULONG i = 0; i < count; i++)
    {
        out[i].MakeCode = raw[i].MakeCode;
        out[i].Flags = raw[i].Flags;
        out[i].UnitId = raw[i].UnitId;
    }

    return count;
}
