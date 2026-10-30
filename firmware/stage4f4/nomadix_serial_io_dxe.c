#ifndef NOMADIX_STAGE4F4_ENTRY
#define NOMADIX_STAGE4F4_ENTRY EfiMain
#endif
#ifndef NOMADIX_ENABLE_STAGE4F4_DIAGNOSTICS
#define NOMADIX_ENABLE_STAGE4F4_DIAGNOSTICS 1
#endif
#define NOMADIX_STAGE4F3_ENTRY Stage4F3EfiMain
#include "../stage4f3/nomadix_serial_diag_dxe.c"
#undef NOMADIX_STAGE4F3_ENTRY
#define EfiMain NOMADIX_STAGE4F4_ENTRY


#define EFI_INVALID_PARAMETER EFIERR(2)
#define EFI_TIMEOUT EFIERR(18)

#define EFI_SERIAL_CLEAR_TO_SEND 0x00000010U
#define EFI_SERIAL_DATA_SET_READY 0x00000020U
#define EFI_SERIAL_RING_INDICATE 0x00000040U
#define EFI_SERIAL_CARRIER_DETECT 0x00000080U
#define EFI_SERIAL_INPUT_BUFFER_EMPTY 0x00000100U
#define EFI_SERIAL_OUTPUT_BUFFER_EMPTY 0x00000200U
#define EFI_SERIAL_REQUEST_TO_SEND 0x00000002U
#define EFI_SERIAL_DATA_TERMINAL_READY 0x00000001U
#define EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE 0x00001000U

#define UART_LSR_DATA_READY 0x01U
#define UART_LSR_THR_EMPTY 0x20U
#define UART_LSR_TRANSMITTER_EMPTY 0x40U

#ifndef NOMADIX_TRACE_SET_ATTRIBUTES_INPUT
#define NOMADIX_TRACE_SET_ATTRIBUTES_INPUT( \
    BaudRate, ReceiveFifoDepth, Timeout, Parity, DataBits, StopBits \
) ((void)0)
#endif
#ifndef NOMADIX_TRACE_SET_ATTRIBUTES_RESULT
#define NOMADIX_TRACE_SET_ATTRIBUTES_RESULT(Status) ((void)0)
#endif
#ifndef NOMADIX_TRACE_SET_CONTROL_INPUT
#define NOMADIX_TRACE_SET_CONTROL_INPUT(Control) ((void)0)
#endif
#ifndef NOMADIX_TRACE_SET_CONTROL_RESULT
#define NOMADIX_TRACE_SET_CONTROL_RESULT(Status) ((void)0)
#endif
#ifndef NOMADIX_SERIAL_PROTOCOL_INTERFACE
#define NOMADIX_SERIAL_PROTOCOL_INTERFACE (&mCom1Protocol)
#endif

typedef struct EFI_SERIAL_IO_PROTOCOL EFI_SERIAL_IO_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_SERIAL_RESET)(
    EFI_SERIAL_IO_PROTOCOL *This
);
typedef EFI_STATUS (EFIAPI *EFI_SERIAL_SET_ATTRIBUTES)(
    EFI_SERIAL_IO_PROTOCOL *This,
    uint64_t BaudRate,
    uint32_t ReceiveFifoDepth,
    uint32_t Timeout,
    uint32_t Parity,
    uint8_t DataBits,
    uint32_t StopBits
);
typedef EFI_STATUS (EFIAPI *EFI_SERIAL_SET_CONTROL)(
    EFI_SERIAL_IO_PROTOCOL *This,
    uint32_t Control
);
typedef EFI_STATUS (EFIAPI *EFI_SERIAL_GET_CONTROL)(
    EFI_SERIAL_IO_PROTOCOL *This,
    uint32_t *Control
);
typedef EFI_STATUS (EFIAPI *EFI_SERIAL_WRITE)(
    EFI_SERIAL_IO_PROTOCOL *This,
    UINTN *BufferSize,
    void *Buffer
);
typedef EFI_STATUS (EFIAPI *EFI_SERIAL_READ)(
    EFI_SERIAL_IO_PROTOCOL *This,
    UINTN *BufferSize,
    void *Buffer
);
typedef EFI_STATUS (EFIAPI *EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES)(
    EFI_HANDLE *Handle,
    ...
);
typedef EFI_STATUS (EFIAPI *EFI_STALL)(UINTN Microseconds);

typedef struct {
    uint32_t ControlMask;
    uint32_t Timeout;
    uint64_t BaudRate;
    uint32_t ReceiveFifoDepth;
    uint32_t DataBits;
    uint32_t Parity;
    uint32_t StopBits;
} EFI_SERIAL_IO_MODE;

struct EFI_SERIAL_IO_PROTOCOL {
    uint32_t Revision;
    EFI_SERIAL_RESET Reset;
    EFI_SERIAL_SET_ATTRIBUTES SetAttributes;
    EFI_SERIAL_SET_CONTROL SetControl;
    EFI_SERIAL_GET_CONTROL GetControl;
    EFI_SERIAL_WRITE Write;
    EFI_SERIAL_READ Read;
    EFI_SERIAL_IO_MODE *Mode;
    const EFI_GUID *DeviceTypeGuid;
};

typedef struct {
    EFI_BOOT_SERVICES Base;
    void *LocateProtocol;
    EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES
        InstallMultipleProtocolInterfaces;
} EFI_BOOT_SERVICES_EXTENDED;

#pragma pack(push, 1)
typedef struct {
    uint8_t Type;
    uint8_t SubType;
    uint16_t Length;
} EFI_DEVICE_PATH_HEADER;

typedef struct {
    EFI_DEVICE_PATH_HEADER Header;
    EFI_GUID Guid;
} VENDOR_DEVICE_PATH;

typedef struct {
    EFI_DEVICE_PATH_HEADER Header;
    uint32_t Reserved;
    uint64_t BaudRate;
    uint8_t DataBits;
    uint8_t Parity;
    uint8_t StopBits;
} UART_DEVICE_PATH;

typedef struct {
    VENDOR_DEVICE_PATH Controller;
    UART_DEVICE_PATH Uart;
    EFI_DEVICE_PATH_HEADER End;
} NOMADIX_SERIAL_DEVICE_PATH;
#pragma pack(pop)

typedef char AssertSerialModeSize[
    sizeof(EFI_SERIAL_IO_MODE) == 32 ? 1 : -1
];
typedef char AssertSerialProtocolModeOffset[
    offsetof(EFI_SERIAL_IO_PROTOCOL, Mode) == 56 ? 1 : -1
];
typedef char AssertInstallMultipleOffset[
    offsetof(
        EFI_BOOT_SERVICES_EXTENDED,
        InstallMultipleProtocolInterfaces
    ) == 328 ? 1 : -1
];
typedef char AssertSerialDevicePathSize[
    sizeof(NOMADIX_SERIAL_DEVICE_PATH) == 43 ? 1 : -1
];

static const EFI_GUID mNomadixCom1ControllerGuid = {
    0xA5C868D3,
    0x3E2D,
    0x4F65,
    {0xA9, 0xE8, 0xF4, 0x8A, 0x2B, 0x9B, 0xD7, 0x52}
};
static const EFI_GUID mSimpleTextOutProtocolGuid = {
    0x387477C2,
    0x69C7,
    0x11D2,
    {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}
};

static EFI_STATUS EFIAPI Com1Reset(EFI_SERIAL_IO_PROTOCOL *This);
static EFI_STATUS EFIAPI Com1SetAttributes(
    EFI_SERIAL_IO_PROTOCOL *This,
    uint64_t BaudRate,
    uint32_t ReceiveFifoDepth,
    uint32_t Timeout,
    uint32_t Parity,
    uint8_t DataBits,
    uint32_t StopBits
);
static EFI_STATUS EFIAPI Com1SetControl(
    EFI_SERIAL_IO_PROTOCOL *This,
    uint32_t Control
);
static EFI_STATUS EFIAPI Com1GetControl(
    EFI_SERIAL_IO_PROTOCOL *This,
    uint32_t *Control
);
static EFI_STATUS EFIAPI Com1Write(
    EFI_SERIAL_IO_PROTOCOL *This,
    UINTN *BufferSize,
    void *Buffer
);
static EFI_STATUS EFIAPI Com1Read(
    EFI_SERIAL_IO_PROTOCOL *This,
    UINTN *BufferSize,
    void *Buffer
);

static EFI_SERIAL_IO_MODE mCom1Mode = {
    EFI_SERIAL_CLEAR_TO_SEND |
        EFI_SERIAL_DATA_SET_READY |
        EFI_SERIAL_RING_INDICATE |
        EFI_SERIAL_CARRIER_DETECT |
        EFI_SERIAL_INPUT_BUFFER_EMPTY |
        EFI_SERIAL_OUTPUT_BUFFER_EMPTY |
        EFI_SERIAL_REQUEST_TO_SEND |
        EFI_SERIAL_DATA_TERMINAL_READY |
        EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE,
    1000000U,
    115200U,
    16U,
    8U,
    1U,
    1U
};

static EFI_SERIAL_IO_PROTOCOL mCom1Protocol = {
    0x00010000U,
    Com1Reset,
    Com1SetAttributes,
    Com1SetControl,
    Com1GetControl,
    Com1Write,
    Com1Read,
    &mCom1Mode,
    0
};

static NOMADIX_SERIAL_DEVICE_PATH mCom1DevicePath = {
    {
        {0x01, 0x04, sizeof(VENDOR_DEVICE_PATH)},
        {
            0xA5C868D3,
            0x3E2D,
            0x4F65,
            {0xA9, 0xE8, 0xF4, 0x8A, 0x2B, 0x9B, 0xD7, 0x52}
        }
    },
    {
        {0x03, 0x0E, sizeof(UART_DEVICE_PATH)},
        0,
        115200U,
        8U,
        1U,
        1U
    },
    {0x7F, 0xFF, sizeof(EFI_DEVICE_PATH_HEADER)}
};

static EFI_HANDLE mCom1Handle;
static uint8_t mTextOutDumped;


static EFI_STATUS ProgramCom1(
    uint64_t BaudRate,
    uint32_t Parity,
    uint8_t DataBits,
    uint32_t StopBits
)
{
    uint64_t Divisor;
    uint8_t LineControl;

    if (BaudRate == 0U) {
        BaudRate = 115200U;
    }
    if (BaudRate > 115200U || 115200U % BaudRate != 0U) {
        return EFI_INVALID_PARAMETER;
    }
    Divisor = 115200U / BaudRate;
    if (Divisor == 0U || Divisor > 0xFFFFU) {
        return EFI_INVALID_PARAMETER;
    }

    if (DataBits == 0U) {
        DataBits = 8U;
    }
    if (DataBits < 5U || DataBits > 8U) {
        return EFI_INVALID_PARAMETER;
    }
    LineControl = (uint8_t)(DataBits - 5U);

    if (Parity == 0U) {
        Parity = 1U;
    }
    switch (Parity) {
        case 1U:
            break;
        case 2U:
            LineControl |= 0x18U;
            break;
        case 3U:
            LineControl |= 0x08U;
            break;
        case 4U:
            LineControl |= 0x28U;
            break;
        case 5U:
            LineControl |= 0x38U;
            break;
        default:
            return EFI_INVALID_PARAMETER;
    }

    if (StopBits == 0U) {
        StopBits = 1U;
    }
    if (StopBits == 2U) {
        if (DataBits != 5U) {
            return EFI_INVALID_PARAMETER;
        }
        LineControl |= 0x04U;
    } else if (StopBits == 3U) {
        if (DataBits == 5U) {
            return EFI_INVALID_PARAMETER;
        }
        LineControl |= 0x04U;
    } else if (StopBits != 1U) {
        return EFI_INVALID_PARAMETER;
    }

    __outbyte(COM1_BASE + 1, 0x00);
    __outbyte(COM1_BASE + 3, (uint8_t)(LineControl | 0x80U));
    __outbyte(COM1_BASE + 0, (uint8_t)Divisor);
    __outbyte(COM1_BASE + 1, (uint8_t)(Divisor >> 8));
    __outbyte(COM1_BASE + 3, LineControl);
    __outbyte(COM1_BASE + 2, 0xC7);
    __outbyte(COM1_BASE + 4, 0x0B);
    return EFI_SUCCESS;
}


static EFI_STATUS EFIAPI Com1Reset(EFI_SERIAL_IO_PROTOCOL *This)
{
    if (This == 0 || This->Mode == 0) {
        return EFI_INVALID_PARAMETER;
    }
    return ProgramCom1(
        This->Mode->BaudRate,
        This->Mode->Parity,
        (uint8_t)This->Mode->DataBits,
        This->Mode->StopBits
    );
}


static EFI_STATUS EFIAPI Com1SetAttributes(
    EFI_SERIAL_IO_PROTOCOL *This,
    uint64_t BaudRate,
    uint32_t ReceiveFifoDepth,
    uint32_t Timeout,
    uint32_t Parity,
    uint8_t DataBits,
    uint32_t StopBits
)
{
    EFI_STATUS Status;

    NOMADIX_TRACE_SET_ATTRIBUTES_INPUT(
        BaudRate,
        ReceiveFifoDepth,
        Timeout,
        Parity,
        DataBits,
        StopBits
    );
    if (This == 0 || This->Mode == 0) {
        NOMADIX_TRACE_SET_ATTRIBUTES_RESULT(EFI_INVALID_PARAMETER);
        return EFI_INVALID_PARAMETER;
    }
    if (BaudRate == 0U) {
        BaudRate = 115200U;
    }
    if (ReceiveFifoDepth == 0U) {
        ReceiveFifoDepth = 16U;
    }
    if (ReceiveFifoDepth > 16U) {
        NOMADIX_TRACE_SET_ATTRIBUTES_RESULT(EFI_INVALID_PARAMETER);
        return EFI_INVALID_PARAMETER;
    }
    if (Timeout == 0U) {
        Timeout = 1000000U;
    }
    if (Parity == 0U) {
        Parity = 1U;
    }
    if (DataBits == 0U) {
        DataBits = 8U;
    }
    if (StopBits == 0U) {
        StopBits = 1U;
    }

    Status = ProgramCom1(BaudRate, Parity, DataBits, StopBits);
    if (Status != EFI_SUCCESS) {
        NOMADIX_TRACE_SET_ATTRIBUTES_RESULT(Status);
        return Status;
    }
    This->Mode->BaudRate = BaudRate;
    This->Mode->ReceiveFifoDepth = ReceiveFifoDepth;
    This->Mode->Timeout = Timeout;
    This->Mode->Parity = Parity;
    This->Mode->DataBits = DataBits;
    This->Mode->StopBits = StopBits;
    mCom1DevicePath.Uart.BaudRate = BaudRate;
    mCom1DevicePath.Uart.Parity = (uint8_t)Parity;
    mCom1DevicePath.Uart.DataBits = DataBits;
    mCom1DevicePath.Uart.StopBits = (uint8_t)StopBits;
    NOMADIX_TRACE_SET_ATTRIBUTES_RESULT(EFI_SUCCESS);
    return EFI_SUCCESS;
}


static EFI_STATUS EFIAPI Com1SetControl(
    EFI_SERIAL_IO_PROTOCOL *This,
    uint32_t Control
)
{
    uint8_t ModemControl;
    uint32_t Settable;

    NOMADIX_TRACE_SET_CONTROL_INPUT(Control);
    if (This == 0) {
        NOMADIX_TRACE_SET_CONTROL_RESULT(EFI_INVALID_PARAMETER);
        return EFI_INVALID_PARAMETER;
    }
    Settable = EFI_SERIAL_DATA_TERMINAL_READY |
               EFI_SERIAL_REQUEST_TO_SEND |
               EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE;
    if ((Control & ~Settable) != 0U) {
        NOMADIX_TRACE_SET_CONTROL_RESULT(EFI_UNSUPPORTED);
        return EFI_UNSUPPORTED;
    }
    ModemControl = 0x08U;
    if ((Control & EFI_SERIAL_DATA_TERMINAL_READY) != 0U) {
        ModemControl |= 0x01U;
    }
    if ((Control & EFI_SERIAL_REQUEST_TO_SEND) != 0U) {
        ModemControl |= 0x02U;
    }
    if ((Control & EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE) != 0U) {
        ModemControl |= 0x10U;
    }
    __outbyte(COM1_BASE + 4, ModemControl);
    NOMADIX_TRACE_SET_CONTROL_RESULT(EFI_SUCCESS);
    return EFI_SUCCESS;
}


static EFI_STATUS EFIAPI Com1GetControl(
    EFI_SERIAL_IO_PROTOCOL *This,
    uint32_t *Control
)
{
    uint8_t LineStatus;
    uint8_t ModemControl;
    uint8_t ModemStatus;
    uint32_t Value;

    if (This == 0 || Control == 0) {
        return EFI_INVALID_PARAMETER;
    }
    LineStatus = __inbyte(COM1_BASE + 5);
    ModemStatus = __inbyte(COM1_BASE + 6);
    ModemControl = __inbyte(COM1_BASE + 4);
    Value = 0;
    if ((ModemStatus & 0x10U) != 0U) {
        Value |= EFI_SERIAL_CLEAR_TO_SEND;
    }
    if ((ModemStatus & 0x20U) != 0U) {
        Value |= EFI_SERIAL_DATA_SET_READY;
    }
    if ((ModemStatus & 0x40U) != 0U) {
        Value |= EFI_SERIAL_RING_INDICATE;
    }
    if ((ModemStatus & 0x80U) != 0U) {
        Value |= EFI_SERIAL_CARRIER_DETECT;
    }
    if ((LineStatus & UART_LSR_DATA_READY) == 0U) {
        Value |= EFI_SERIAL_INPUT_BUFFER_EMPTY;
    }
    if ((LineStatus & UART_LSR_TRANSMITTER_EMPTY) != 0U) {
        Value |= EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
    }
    if ((ModemControl & 0x01U) != 0U) {
        Value |= EFI_SERIAL_DATA_TERMINAL_READY;
    }
    if ((ModemControl & 0x02U) != 0U) {
        Value |= EFI_SERIAL_REQUEST_TO_SEND;
    }
    if ((ModemControl & 0x10U) != 0U) {
        Value |= EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE;
    }
    *Control = Value;
    return EFI_SUCCESS;
}


static int WaitForCom1(uint8_t Mask, uint8_t Expected, uint32_t Timeout)
{
    EFI_STALL Stall;
    uint32_t Elapsed;

    Stall = (EFI_STALL)mBootServices->Stall;
    for (Elapsed = 0; Elapsed < Timeout; Elapsed += 10U) {
        if ((__inbyte(COM1_BASE + 5) & Mask) == Expected) {
            return 1;
        }
        Stall(10U);
    }
    return 0;
}


static EFI_STATUS EFIAPI Com1Write(
    EFI_SERIAL_IO_PROTOCOL *This,
    UINTN *BufferSize,
    void *Buffer
)
{
    const uint8_t *Bytes;
    UINTN Index;
    UINTN Requested;

    if (This == 0 || BufferSize == 0 || (*BufferSize != 0U && Buffer == 0)) {
        return EFI_INVALID_PARAMETER;
    }
    Requested = *BufferSize;
    Bytes = (const uint8_t *)Buffer;
    for (Index = 0; Index < Requested; ++Index) {
        if (!WaitForCom1(
                UART_LSR_THR_EMPTY,
                UART_LSR_THR_EMPTY,
                This->Mode->Timeout
            )) {
            *BufferSize = Index;
            return EFI_TIMEOUT;
        }
        __outbyte(COM1_BASE, Bytes[Index]);
    }
    return EFI_SUCCESS;
}


static EFI_STATUS EFIAPI Com1Read(
    EFI_SERIAL_IO_PROTOCOL *This,
    UINTN *BufferSize,
    void *Buffer
)
{
    uint8_t *Bytes;
    UINTN Index;
    UINTN Requested;

    if (This == 0 || BufferSize == 0 || (*BufferSize != 0U && Buffer == 0)) {
        return EFI_INVALID_PARAMETER;
    }
    Requested = *BufferSize;
    Bytes = (uint8_t *)Buffer;
    for (Index = 0; Index < Requested; ++Index) {
        if (!WaitForCom1(
                UART_LSR_DATA_READY,
                UART_LSR_DATA_READY,
                This->Mode->Timeout
            )) {
            *BufferSize = Index;
            return EFI_TIMEOUT;
        }
        Bytes[Index] = __inbyte(COM1_BASE);
    }
    return EFI_SUCCESS;
}


static void EFIAPI DumpTextOutHandles(EFI_EVENT Event, void *Context)
{
    EFI_HANDLE *Handles;
    EFI_STATUS Status;
    UINTN Count;
    UINTN Index;

    (void)Event;
    (void)Context;
    if (mTextOutDumped != 0U) {
        return;
    }
    mTextOutDumped = 1U;
    SerialWriteString("\r\n[NOMADIX-4F4] TEXT-OUT-DIAG\r\n");
    Handles = 0;
    Count = 0;
    Status = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mSimpleTextOutProtocolGuid,
        0,
        &Count,
        &Handles
    );
    SerialWriteString("TEXT_OUT LOCATE=");
    SerialWriteHex64(Status);
    SerialWriteString(" COUNT=");
    SerialWriteHex64(Count);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        return;
    }
    for (Index = 0; Index < Count && Index < 16U; ++Index) {
        void *DevicePath;

        DevicePath = 0;
        Status = mBootServices->HandleProtocol(
            Handles[Index],
            &mDevicePathProtocolGuid,
            &DevicePath
        );
        SerialWriteString("TEXT[");
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
    EFI_BOOT_SERVICES_EXTENDED *BootServices;
    EFI_EVENT LegacyBootEvent;
    EFI_EVENT ReadyToBootEvent;
    EFI_STATUS LegacyStatus;
    EFI_STATUS ReadyStatus;
    EFI_STATUS Status;

    Status = Stage4F3EfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    BootServices = (EFI_BOOT_SERVICES_EXTENDED *)SystemTable->BootServices;
    mCom1Handle = 0;
    Status = BootServices->InstallMultipleProtocolInterfaces(
        &mCom1Handle,
        &mSerialIoProtocolGuid,
        NOMADIX_SERIAL_PROTOCOL_INTERFACE,
        &mDevicePathProtocolGuid,
        &mCom1DevicePath,
        0
    );
    SerialWriteString("[NOMADIX-4F4] PUBLISH=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        return Status;
    }

#if NOMADIX_ENABLE_STAGE4F4_DIAGNOSTICS
    ReadyToBootEvent = 0;
    ReadyStatus = mBootServices->CreateEvent(
        EVT_SIGNAL_READY_TO_BOOT,
        TPL_CALLBACK,
        DumpTextOutHandles,
        0,
        &ReadyToBootEvent
    );
    LegacyBootEvent = 0;
    LegacyStatus = mBootServices->CreateEvent(
        EVT_SIGNAL_LEGACY_BOOT,
        TPL_CALLBACK,
        DumpTextOutHandles,
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
        return EFI_DEVICE_ERROR;
    }
    return EFI_SUCCESS;
}
#undef EfiMain
