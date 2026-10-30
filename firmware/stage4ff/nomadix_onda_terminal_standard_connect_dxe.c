#include <stdint.h>

#ifndef NOMADIX_STAGE4FF_ENTRY
#define NOMADIX_STAGE4FF_ENTRY EfiMain
#endif
#ifndef NOMADIX_STAGE_PREFIX
#define NOMADIX_STAGE_PREFIX "[NOMADIX-4FF]"
#endif
#ifndef NOMADIX_PREPARE_AMI_PRIVATE
#define NOMADIX_PREPARE_AMI_PRIVATE(Container) ((void)0)
#endif
#ifndef NOMADIX_CONNECT_ONDA_TERMINAL
#define NOMADIX_CONNECT_ONDA_TERMINAL(Status, RemainingDevicePath) \
    do { \
        ConnectController = \
            (EFI_CONNECT_CONTROLLER)mBootServices->ConnectController; \
        (Status) = ConnectController( \
            mCom1Handle, \
            0, \
            (RemainingDevicePath), \
            1U \
        ); \
    } while (0)
#endif
#ifndef NOMADIX_CAPTURE_SYSTEM_TABLE
#define NOMADIX_CAPTURE_SYSTEM_TABLE(SystemTable) ((void)0)
#endif
static void *GetAmiSerialProtocolInterface(void);

#define NOMADIX_SERIAL_PROTOCOL_INTERFACE GetAmiSerialProtocolInterface()
#define NOMADIX_STAGE4F4_ENTRY Stage4F4EfiMain
#include "../stage4f4/nomadix_serial_io_dxe.c"
#undef NOMADIX_STAGE4F4_ENTRY
#define EfiMain NOMADIX_STAGE4FF_ENTRY

#define AMI_SERIAL_CONTAINER_SIZE 0x130U
#define AMI_SERIAL_SIGNATURE_OFFSET 0x40U
#define AMI_SERIAL_IO_BASE_OFFSET 0xB8U
#define AMI_SERIAL_PROTOCOL_BACKEND_OFFSET 0x108U
#define AMI_SERIAL_REGISTER_BAR_OFFSET 0x110U
#define AMI_SERIAL_ACCESS_WIDTH_OFFSET 0x112U
#define AMI_SERIAL_MMIO_FLAG_OFFSET 0x121U
#define AMI_SERIAL_SIGNATURE 0x4F495324U

typedef EFI_STATUS (EFIAPI *EFI_CONNECT_CONTROLLER)(
    EFI_HANDLE ControllerHandle,
    EFI_HANDLE *DriverImageHandle,
    void *RemainingDevicePath,
    uint8_t Recursive
);

#pragma pack(push, 1)
typedef struct {
    EFI_DEVICE_PATH_HEADER Header;
    EFI_GUID Guid;
    EFI_DEVICE_PATH_HEADER End;
} TERMINAL_REMAINING_DEVICE_PATH;
#pragma pack(pop)

typedef char AssertTerminalRemainingPathSize[
    sizeof(TERMINAL_REMAINING_DEVICE_PATH) == 24 ? 1 : -1
];

static uint64_t mAmiSerialContainer[
    (AMI_SERIAL_CONTAINER_SIZE + sizeof(uint64_t) - 1U) / sizeof(uint64_t)
];
static uint8_t mConnectAttempted;
static TERMINAL_REMAINING_DEVICE_PATH mPcAnsiRemainingPath = {
    {
        0x03,
        0x0A,
        sizeof(VENDOR_DEVICE_PATH)
    },
    {
        0xE0C14753,
        0xF9BE,
        0x11D2,
        {0x9A, 0x0C, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D}
    },
    {
        0x7F,
        0xFF,
        sizeof(EFI_DEVICE_PATH_HEADER)
    }
};


static void CopyBytes(uint8_t *Destination, const uint8_t *Source, UINTN Size)
{
    UINTN Index;

    for (Index = 0; Index < Size; ++Index) {
        Destination[Index] = Source[Index];
    }
}


static void WriteUint16(uint8_t *Destination, uint16_t Value)
{
    Destination[0] = (uint8_t)Value;
    Destination[1] = (uint8_t)(Value >> 8);
}


static void WriteUint32(uint8_t *Destination, uint32_t Value)
{
    Destination[0] = (uint8_t)Value;
    Destination[1] = (uint8_t)(Value >> 8);
    Destination[2] = (uint8_t)(Value >> 16);
    Destination[3] = (uint8_t)(Value >> 24);
}


static void *GetAmiSerialProtocolInterface(void)
{
    return (void *)mAmiSerialContainer;
}


static EFI_STATUS PrepareAmiSerialContainer(void)
{
    uint8_t *Container;
    volatile uint8_t *VolatileContainer;
    UINTN Index;

    Container = (uint8_t *)mAmiSerialContainer;
    VolatileContainer = (volatile uint8_t *)mAmiSerialContainer;
    for (Index = 0; Index < AMI_SERIAL_CONTAINER_SIZE; ++Index) {
        VolatileContainer[Index] = 0;
    }
    CopyBytes(
        Container,
        (const uint8_t *)&mCom1Protocol,
        sizeof(mCom1Protocol)
    );
#ifndef NOMADIX_DEFER_ONDA_TERMINAL_ARM
    WriteUint32(
        Container + AMI_SERIAL_SIGNATURE_OFFSET,
        AMI_SERIAL_SIGNATURE
    );
#endif
    WriteUint16(
        Container + AMI_SERIAL_IO_BASE_OFFSET,
        COM1_BASE
    );
    Container[AMI_SERIAL_PROTOCOL_BACKEND_OFFSET] = 0;
    Container[AMI_SERIAL_REGISTER_BAR_OFFSET] = 0;
    Container[AMI_SERIAL_ACCESS_WIDTH_OFFSET] = 0;
    Container[AMI_SERIAL_MMIO_FLAG_OFFSET] = 0;
    NOMADIX_PREPARE_AMI_PRIVATE(Container);

    if (
#ifndef NOMADIX_DEFER_ONDA_TERMINAL_ARM
        *(uint32_t *)(Container + AMI_SERIAL_SIGNATURE_OFFSET) !=
            AMI_SERIAL_SIGNATURE ||
#endif
        *(uint16_t *)(Container + AMI_SERIAL_IO_BASE_OFFSET) != COM1_BASE
    ) {
        return EFI_DEVICE_ERROR;
    }
    return EFI_SUCCESS;
}


static EFI_STATUS ConvertCom1DevicePathToAcpi(void)
{
    uint8_t *Path;
    uint8_t Tail[23];
    UINTN Index;

    Path = (uint8_t *)&mCom1DevicePath;
    if (
        Path[0] != 0x01U ||
        Path[1] != 0x04U ||
        Path[2] != sizeof(VENDOR_DEVICE_PATH) ||
        Path[3] != 0x00U ||
        Path[20] != 0x03U ||
        Path[21] != 0x0EU ||
        Path[22] != sizeof(UART_DEVICE_PATH) ||
        Path[23] != 0x00U ||
        Path[39] != 0x7FU ||
        Path[40] != 0xFFU
    ) {
        return EFI_UNSUPPORTED;
    }
    for (Index = 0; Index < sizeof(Tail); ++Index) {
        Tail[Index] = Path[sizeof(VENDOR_DEVICE_PATH) + Index];
    }
    Path[0] = 0x02U;
    Path[1] = 0x01U;
    Path[2] = 0x0CU;
    Path[3] = 0;
    WriteUint32(Path + 4, 0x050141D0U);
    WriteUint32(Path + 8, 0);
    CopyBytes(Path + 12, Tail, sizeof(Tail));
    return EFI_SUCCESS;
}


static void EFIAPI ConnectOndaTerminal(EFI_EVENT Event, void *Context)
{
#ifndef NOMADIX_CUSTOM_CONNECT_ONDA_TERMINAL
    EFI_CONNECT_CONTROLLER ConnectController;
#endif
    EFI_STATUS Status;

    (void)Event;
    (void)Context;
    if (mConnectAttempted != 0U) {
        return;
    }
    mConnectAttempted = 1U;
    SerialConfigure();
#ifdef NOMADIX_DEFER_ONDA_TERMINAL_ARM
    Status = ConvertCom1DevicePathToAcpi();
    if (Status != EFI_SUCCESS) {
        SerialWriteString(
            "\r\n" NOMADIX_STAGE_PREFIX " PATH-CONVERSION-FAILED\r\n"
        );
        return;
    }
    WriteUint32(
        (uint8_t *)mAmiSerialContainer + AMI_SERIAL_SIGNATURE_OFFSET,
        AMI_SERIAL_SIGNATURE
    );
    SerialWriteString(
        "\r\n" NOMADIX_STAGE_PREFIX " DEFERRED-ARMED\r\n"
    );
#endif
    SerialWriteString(NOMADIX_STAGE_PREFIX " BEFORE-CONNECT\r\n");
    NOMADIX_CONNECT_ONDA_TERMINAL(Status, &mPcAnsiRemainingPath);
    SerialConfigure();
    SerialWriteString(NOMADIX_STAGE_PREFIX " CONNECT=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
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

    NOMADIX_CAPTURE_SYSTEM_TABLE(SystemTable);
    Status = PrepareAmiSerialContainer();
    if (Status != EFI_SUCCESS) {
        return Status;
    }
#ifndef NOMADIX_DEFER_ONDA_TERMINAL_ARM
    Status = ConvertCom1DevicePathToAcpi();
    if (Status != EFI_SUCCESS) {
        return Status;
    }
#endif
    Status = Stage4F4EfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    SerialWriteString(NOMADIX_STAGE_PREFIX " ARMED\r\n");

    ReadyToBootEvent = 0;
    ReadyStatus = mBootServices->CreateEvent(
        EVT_SIGNAL_READY_TO_BOOT,
        TPL_CALLBACK,
        ConnectOndaTerminal,
        0,
        &ReadyToBootEvent
    );
    LegacyBootEvent = 0;
    LegacyStatus = mBootServices->CreateEvent(
        EVT_SIGNAL_LEGACY_BOOT,
        TPL_CALLBACK,
        ConnectOndaTerminal,
        0,
        &LegacyBootEvent
    );
    if (ReadyStatus != EFI_SUCCESS && LegacyStatus != EFI_SUCCESS) {
        return EFI_DEVICE_ERROR;
    }
    return EFI_SUCCESS;
}
#undef EfiMain
