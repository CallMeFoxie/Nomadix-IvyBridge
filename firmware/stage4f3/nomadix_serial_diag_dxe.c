#ifndef NOMADIX_STAGE4F3_ENTRY
#define NOMADIX_STAGE4F3_ENTRY EfiMain
#endif
#ifndef NOMADIX_ENABLE_STAGE4F3_DIAGNOSTICS
#define NOMADIX_ENABLE_STAGE4F3_DIAGNOSTICS 1
#endif
#ifndef NOMADIX_ENABLE_DIAGNOSTIC_LOG
#define NOMADIX_ENABLE_DIAGNOSTIC_LOG 1
#endif
#define EfiMain Stage4F2EfiMain
#include "../stage4f2/nomadix_platform_dxe.c"
#undef EfiMain
#define EfiMain NOMADIX_STAGE4F3_ENTRY
#include <stddef.h>


typedef uint64_t UINTN;
typedef uint64_t EFI_TPL;
typedef void *EFI_EVENT;

typedef struct {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
} EFI_GUID;

typedef struct {
    uint64_t Signature;
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t CRC32;
    uint32_t Reserved;
} EFI_TABLE_HEADER;

typedef void (EFIAPI *EFI_EVENT_NOTIFY)(EFI_EVENT Event, void *Context);
typedef EFI_STATUS (EFIAPI *EFI_CREATE_EVENT)(
    uint32_t Type,
    EFI_TPL NotifyTpl,
    EFI_EVENT_NOTIFY NotifyFunction,
    void *NotifyContext,
    EFI_EVENT *Event
);
typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(void *Buffer);
typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(
    EFI_HANDLE Handle,
    const EFI_GUID *Protocol,
    void **Interface
);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE_BUFFER)(
    uint32_t SearchType,
    const EFI_GUID *Protocol,
    void *SearchKey,
    UINTN *NoHandles,
    EFI_HANDLE **Buffer
);

typedef struct {
    EFI_TABLE_HEADER Hdr;
    void *RaiseTPL;
    void *RestoreTPL;
    void *AllocatePages;
    void *FreePages;
    void *GetMemoryMap;
    void *AllocatePool;
    EFI_FREE_POOL FreePool;
    EFI_CREATE_EVENT CreateEvent;
    void *SetTimer;
    void *WaitForEvent;
    void *SignalEvent;
    void *CloseEvent;
    void *CheckEvent;
    void *InstallProtocolInterface;
    void *ReinstallProtocolInterface;
    void *UninstallProtocolInterface;
    EFI_HANDLE_PROTOCOL HandleProtocol;
    void *Reserved;
    void *RegisterProtocolNotify;
    void *LocateHandle;
    void *LocateDevicePath;
    void *InstallConfigurationTable;
    void *LoadImage;
    void *StartImage;
    void *Exit;
    void *UnloadImage;
    void *ExitBootServices;
    void *GetNextMonotonicCount;
    void *Stall;
    void *SetWatchdogTimer;
    void *ConnectController;
    void *DisconnectController;
    void *OpenProtocol;
    void *CloseProtocol;
    void *OpenProtocolInformation;
    void *ProtocolsPerHandle;
    EFI_LOCATE_HANDLE_BUFFER LocateHandleBuffer;
} EFI_BOOT_SERVICES;

struct EFI_SYSTEM_TABLE {
    EFI_TABLE_HEADER Hdr;
    uint16_t *FirmwareVendor;
    uint32_t FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    void *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    void *ConOut;
    EFI_HANDLE StandardErrorHandle;
    void *StdErr;
    void *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
};

typedef char AssertFreePoolOffset[
    offsetof(EFI_BOOT_SERVICES, FreePool) == 72 ? 1 : -1
];
typedef char AssertCreateEventOffset[
    offsetof(EFI_BOOT_SERVICES, CreateEvent) == 80 ? 1 : -1
];
typedef char AssertHandleProtocolOffset[
    offsetof(EFI_BOOT_SERVICES, HandleProtocol) == 152 ? 1 : -1
];
typedef char AssertLocateHandleBufferOffset[
    offsetof(EFI_BOOT_SERVICES, LocateHandleBuffer) == 312 ? 1 : -1
];
typedef char AssertBootServicesOffset[
    offsetof(EFI_SYSTEM_TABLE, BootServices) == 96 ? 1 : -1
];

#define COM1_BASE 0x3F8
#define COM_LSR_THRE 0x20
#define EFI_BY_PROTOCOL 2
#define EVT_SIGNAL_READY_TO_BOOT 0x00000203U
#define EVT_SIGNAL_LEGACY_BOOT 0x00000204U
#define TPL_CALLBACK 8

static const EFI_GUID mSerialIoProtocolGuid = {
    0xBB25CF6F,
    0xF1D4,
    0x11D2,
    {0x9A, 0x0C, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0xFD}
};
static const EFI_GUID mDevicePathProtocolGuid = {
    0x09576E91,
    0x6D3F,
    0x11D2,
    {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}
};

static EFI_BOOT_SERVICES *mBootServices;
static uint8_t mDumped;


static void SerialConfigure(void)
{
    __outbyte(COM1_BASE + 1, 0x00);
    __outbyte(COM1_BASE + 3, 0x80);
    __outbyte(COM1_BASE + 0, 0x01);
    __outbyte(COM1_BASE + 1, 0x00);
    __outbyte(COM1_BASE + 3, 0x03);
    __outbyte(COM1_BASE + 2, 0xC7);
    __outbyte(COM1_BASE + 4, 0x0B);
}


static void SerialWriteByte(uint8_t Value)
{
#if NOMADIX_ENABLE_DIAGNOSTIC_LOG
    uint32_t Timeout;

    for (Timeout = 0; Timeout < 1000000U; ++Timeout) {
        if ((__inbyte(COM1_BASE + 5) & COM_LSR_THRE) != 0U) {
            __outbyte(COM1_BASE, Value);
            return;
        }
    }
#else
    (void)Value;
#endif
}


static void SerialWriteString(const char *Text)
{
    while (*Text != '\0') {
        SerialWriteByte((uint8_t)*Text);
        ++Text;
    }
}


static void SerialWriteHexNibble(uint8_t Value)
{
    Value &= 0x0FU;
    SerialWriteByte((uint8_t)(Value < 10U ? '0' + Value : 'A' + Value - 10U));
}


static void SerialWriteHex8(uint8_t Value)
{
    SerialWriteHexNibble((uint8_t)(Value >> 4));
    SerialWriteHexNibble(Value);
}


static void SerialWriteHex64(uint64_t Value)
{
    int32_t Shift;

    for (Shift = 60; Shift >= 0; Shift -= 4) {
        SerialWriteHexNibble((uint8_t)(Value >> Shift));
    }
}


static uint16_t DevicePathNodeLength(const uint8_t *Node)
{
    return (uint16_t)(Node[2] | ((uint16_t)Node[3] << 8));
}


static void DumpDevicePath(const uint8_t *Path)
{
    uint32_t Total;

    for (Total = 0; Total < 1024U;) {
        uint16_t NodeLength = DevicePathNodeLength(Path + Total);
        uint16_t Index;

        if (NodeLength < 4U || Total + NodeLength > 1024U) {
            SerialWriteString("INVALID");
            return;
        }
        for (Index = 0; Index < NodeLength; ++Index) {
            SerialWriteHex8(Path[Total + Index]);
        }
        if (Path[Total] == 0x7FU &&
            Path[Total + 1] == 0xFFU &&
            NodeLength == 4U) {
            return;
        }
        Total += NodeLength;
    }
    SerialWriteString("TOO-LONG");
}


static void EFIAPI DumpSerialHandles(EFI_EVENT Event, void *Context)
{
    EFI_HANDLE *Handles;
    EFI_STATUS Status;
    UINTN Count;
    UINTN Index;

    (void)Event;
    (void)Context;

    if (mDumped != 0U) {
        return;
    }
    mDumped = 1U;
    SerialConfigure();
    SerialWriteString("\r\n[NOMADIX-4F3] SERIAL-DIAG\r\n");

    Handles = 0;
    Count = 0;
    Status = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mSerialIoProtocolGuid,
        0,
        &Count,
        &Handles
    );
    SerialWriteString("LOCATE=");
    SerialWriteHex64(Status);
    SerialWriteString(" COUNT=");
    SerialWriteHex64(Count);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        return;
    }

    for (Index = 0; Index < Count; ++Index) {
        void *DevicePath;

        DevicePath = 0;
        Status = mBootServices->HandleProtocol(
            Handles[Index],
            &mDevicePathProtocolGuid,
            &DevicePath
        );
        SerialWriteString("SERIAL[");
        SerialWriteHex64(Index);
        SerialWriteString("] DP_STATUS=");
        SerialWriteHex64(Status);
        SerialWriteString(" DP=");
        if (Status == EFI_SUCCESS && DevicePath != 0) {
            DumpDevicePath((const uint8_t *)DevicePath);
        }
        SerialWriteString("\r\n");
    }
    mBootServices->FreePool(Handles);
}


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_EVENT LegacyBootEvent;
    EFI_EVENT ReadyToBootEvent;
    EFI_STATUS LegacyStatus;
    EFI_STATUS ReadyStatus;
    EFI_STATUS Status;

    Status = Stage4F2EfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    if (SystemTable == 0 || SystemTable->BootServices == 0) {
        return EFI_DEVICE_ERROR;
    }

    mBootServices = SystemTable->BootServices;
    SerialConfigure();
    SerialWriteString("\r\n[NOMADIX-4F3] ARMED\r\n");

#if NOMADIX_ENABLE_STAGE4F3_DIAGNOSTICS
    ReadyToBootEvent = 0;
    ReadyStatus = mBootServices->CreateEvent(
        EVT_SIGNAL_READY_TO_BOOT,
        TPL_CALLBACK,
        DumpSerialHandles,
        0,
        &ReadyToBootEvent
    );
    LegacyBootEvent = 0;
    LegacyStatus = mBootServices->CreateEvent(
        EVT_SIGNAL_LEGACY_BOOT,
        TPL_CALLBACK,
        DumpSerialHandles,
        0,
        &LegacyBootEvent
    );
#else
    ReadyToBootEvent = 0;
    LegacyBootEvent = 0;
    ReadyStatus = EFI_SUCCESS;
    LegacyStatus = EFI_SUCCESS;
#endif
    if (ReadyStatus != EFI_SUCCESS && LegacyStatus != EFI_SUCCESS) {
        SerialWriteString("[NOMADIX-4F3] EVENT-ERROR READY=");
        SerialWriteHex64(ReadyStatus);
        SerialWriteString(" LEGACY=");
        SerialWriteHex64(LegacyStatus);
        SerialWriteString("\r\n");
        return EFI_DEVICE_ERROR;
    }
    return EFI_SUCCESS;
}
#undef EfiMain
