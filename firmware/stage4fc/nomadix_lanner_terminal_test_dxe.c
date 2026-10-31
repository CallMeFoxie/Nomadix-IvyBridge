#include <stdint.h>

#ifndef NOMADIX_STAGE4FC_ENTRY
#define NOMADIX_STAGE4FC_ENTRY EfiMain
#endif
#ifndef NOMADIX_STAGE_PREFIX
#define NOMADIX_STAGE_PREFIX "[NOMADIX-4FC]"
#endif
#define NOMADIX_STAGE4F4_ENTRY Stage4F4EfiMain
#include "../stage4f4/nomadix_serial_io_dxe.c"
#undef NOMADIX_STAGE4F4_ENTRY
#define EfiMain NOMADIX_STAGE4FC_ENTRY

#define EFI_NOT_FOUND EFIERR(14)

#define LANNER_TERMINAL_EXPECTED_SIZE 0x9E20U
#define ACPI_DEVICE_PATH_TYPE 0x02U
#define ACPI_DEVICE_PATH_SUBTYPE 0x01U
#define ACPI_DEVICE_PATH_LENGTH 0x0CU
#define ACPI_PNP0501_HID 0x050141D0U

typedef struct EFI_DRIVER_BINDING_PROTOCOL EFI_DRIVER_BINDING_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_LOAD_IMAGE)(
    uint8_t BootPolicy,
    EFI_HANDLE ParentImageHandle,
    void *DevicePath,
    void *SourceBuffer,
    UINTN SourceSize,
    EFI_HANDLE *ImageHandle
);
typedef EFI_STATUS (EFIAPI *EFI_START_IMAGE)(
    EFI_HANDLE ImageHandle,
    UINTN *ExitDataSize,
    uint16_t **ExitData
);
typedef EFI_STATUS (EFIAPI *EFI_DRIVER_BINDING_SUPPORTED)(
    EFI_DRIVER_BINDING_PROTOCOL *This,
    EFI_HANDLE ControllerHandle,
    void *RemainingDevicePath
);
typedef EFI_STATUS (EFIAPI *EFI_DRIVER_BINDING_START)(
    EFI_DRIVER_BINDING_PROTOCOL *This,
    EFI_HANDLE ControllerHandle,
    void *RemainingDevicePath
);
typedef EFI_STATUS (EFIAPI *EFI_DRIVER_BINDING_STOP)(
    EFI_DRIVER_BINDING_PROTOCOL *This,
    EFI_HANDLE ControllerHandle,
    UINTN NumberOfChildren,
    EFI_HANDLE *ChildHandleBuffer
);

struct EFI_DRIVER_BINDING_PROTOCOL {
    EFI_DRIVER_BINDING_SUPPORTED Supported;
    EFI_DRIVER_BINDING_START Start;
    EFI_DRIVER_BINDING_STOP Stop;
    uint32_t Version;
    EFI_HANDLE ImageHandle;
    EFI_HANDLE DriverBindingHandle;
};

typedef char AssertDriverBindingSize[
    sizeof(EFI_DRIVER_BINDING_PROTOCOL) == 48 ? 1 : -1
];

#ifndef NOMADIX_LANNER_FROM_FV
extern const uint8_t mLannerTerminalImage[];
extern const uint64_t mLannerTerminalImageSize;
#else
typedef struct EFI_FIRMWARE_VOLUME2_PROTOCOL EFI_FIRMWARE_VOLUME2_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_FV_READ_SECTION)(
    const EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
    const EFI_GUID *NameGuid,
    uint8_t SectionType,
    UINTN SectionInstance,
    void **Buffer,
    UINTN *BufferSize,
    uint32_t *AuthenticationStatus
);
struct EFI_FIRMWARE_VOLUME2_PROTOCOL {
    void *GetVolumeAttributes;
    void *SetVolumeAttributes;
    void *ReadFile;
    EFI_FV_READ_SECTION ReadSection;
    void *WriteFile;
    void *GetNextFile;
    uint32_t KeySize;
    EFI_HANDLE ParentHandle;
    void *GetInfo;
    void *SetInfo;
};
static const EFI_GUID mFirmwareVolume2ProtocolGuid = {
    0x220E73B6,
    0x6BDB,
    0x4413,
    {0x84, 0x05, 0xB9, 0x74, 0xB1, 0x08, 0x61, 0x9A}
};
#ifndef NOMADIX_TERMINAL_FILE_GUID_INITIALIZER
#define NOMADIX_TERMINAL_FILE_GUID_INITIALIZER \
    0x7A08CB98, \
    0xE9BC, \
    0x41C3, \
    {0xBE, 0x19, 0xB3, 0x02, 0xF3, 0xF1, 0xF5, 0x95}
#endif
static const EFI_GUID mTerminalFileGuid = {
    NOMADIX_TERMINAL_FILE_GUID_INITIALIZER
};
#endif

static const EFI_GUID mDriverBindingProtocolGuid = {
    0x18A031AB,
    0xB443,
    0x4D1A,
    {0xA5, 0xC0, 0x0C, 0x09, 0x26, 0x1E, 0x9F, 0x71}
};


static void CopyBytes(uint8_t *Destination, const uint8_t *Source, UINTN Size)
{
    UINTN Index;

    for (Index = 0; Index < Size; ++Index) {
        Destination[Index] = Source[Index];
    }
}


static void WriteUint32(uint8_t *Destination, uint32_t Value)
{
    Destination[0] = (uint8_t)Value;
    Destination[1] = (uint8_t)(Value >> 8);
    Destination[2] = (uint8_t)(Value >> 16);
    Destination[3] = (uint8_t)(Value >> 24);
}


static int HandlesContain(
    EFI_HANDLE *Handles,
    UINTN Count,
    EFI_HANDLE Candidate
)
{
    UINTN Index;

    for (Index = 0; Index < Count; ++Index) {
        if (Handles[Index] == Candidate) {
            return 1;
        }
    }
    return 0;
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
        Path[40] != 0xFFU ||
        Path[41] != sizeof(EFI_DEVICE_PATH_HEADER) ||
        Path[42] != 0x00U
    ) {
        return EFI_UNSUPPORTED;
    }
    for (Index = 0; Index < sizeof(EFI_GUID); ++Index) {
        if (
            Path[sizeof(EFI_DEVICE_PATH_HEADER) + Index] !=
            ((const uint8_t *)&mNomadixCom1ControllerGuid)[Index]
        ) {
            return EFI_UNSUPPORTED;
        }
    }

    CopyBytes(Tail, Path + sizeof(VENDOR_DEVICE_PATH), sizeof(Tail));
    Path[0] = ACPI_DEVICE_PATH_TYPE;
    Path[1] = ACPI_DEVICE_PATH_SUBTYPE;
    Path[2] = ACPI_DEVICE_PATH_LENGTH;
    Path[3] = 0;
    WriteUint32(Path + 4, ACPI_PNP0501_HID);
    WriteUint32(Path + 8, 0);
    CopyBytes(Path + ACPI_DEVICE_PATH_LENGTH, Tail, sizeof(Tail));

    SerialWriteString(NOMADIX_STAGE_PREFIX " ACPI-PNP0501-PATH\r\n");
    return EFI_SUCCESS;
}


static EFI_STATUS LoadLannerTerminal(
    EFI_HANDLE ParentImageHandle,
    EFI_HANDLE *TerminalImageHandle
)
{
#ifdef NOMADIX_LANNER_FROM_FV
    EFI_FIRMWARE_VOLUME2_PROTOCOL *FirmwareVolume;
    EFI_HANDLE *Handles;
    EFI_STATUS ReadStatus;
    UINTN Count;
    UINTN Index;
    uint32_t AuthenticationStatus;
#endif
    EFI_LOAD_IMAGE LoadImage;
#ifndef NOMADIX_LANNER_LOAD_ONLY
    EFI_START_IMAGE StartImage;
#endif
    EFI_STATUS Status;
#ifndef NOMADIX_LANNER_LOAD_ONLY
    UINTN ExitDataSize;
#endif
    void *TerminalImage;
    UINTN TerminalImageSize;
#ifndef NOMADIX_LANNER_LOAD_ONLY
    uint16_t *ExitData;
#endif

#ifdef NOMADIX_LANNER_FROM_FV
    Handles = 0;
    Count = 0;
    ReadStatus = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mFirmwareVolume2ProtocolGuid,
        0,
        &Count,
        &Handles
    );
    TerminalImage = 0;
    TerminalImageSize = 0;
    AuthenticationStatus = 0;
    if (ReadStatus == EFI_SUCCESS) {
        ReadStatus = EFI_NOT_FOUND;
        for (Index = 0; Index < Count; ++Index) {
            FirmwareVolume = 0;
            Status = mBootServices->HandleProtocol(
                Handles[Index],
                &mFirmwareVolume2ProtocolGuid,
                (void **)&FirmwareVolume
            );
            if (Status != EFI_SUCCESS) {
                continue;
            }
            TerminalImage = 0;
            TerminalImageSize = 0;
            AuthenticationStatus = 0;
            ReadStatus = FirmwareVolume->ReadSection(
                FirmwareVolume,
                &mTerminalFileGuid,
                0x10U,
                0,
                &TerminalImage,
                &TerminalImageSize,
                &AuthenticationStatus
            );
            if (ReadStatus == EFI_SUCCESS) {
                break;
            }
        }
        mBootServices->FreePool(Handles);
    }
    SerialWriteString(NOMADIX_STAGE_PREFIX " FV_READ=");
    SerialWriteHex64(ReadStatus);
    SerialWriteString(" AUTH=");
    SerialWriteHex64(AuthenticationStatus);
    SerialWriteString("\r\n");
    if (ReadStatus != EFI_SUCCESS) {
        return ReadStatus;
    }
#else
    TerminalImage = (void *)mLannerTerminalImage;
    TerminalImageSize = (UINTN)mLannerTerminalImageSize;
#endif

    SerialWriteString(NOMADIX_STAGE_PREFIX " PAYLOAD_SIZE=");
    SerialWriteHex64(TerminalImageSize);
    SerialWriteString("\r\n");
    if (TerminalImageSize != LANNER_TERMINAL_EXPECTED_SIZE) {
#ifdef NOMADIX_LANNER_FROM_FV
        mBootServices->FreePool(TerminalImage);
#endif
        return EFI_UNSUPPORTED;
    }

    LoadImage = (EFI_LOAD_IMAGE)mBootServices->LoadImage;
    *TerminalImageHandle = 0;
    Status = LoadImage(
        0,
        ParentImageHandle,
        0,
        TerminalImage,
        TerminalImageSize,
        TerminalImageHandle
    );
#ifdef NOMADIX_LANNER_FROM_FV
    mBootServices->FreePool(TerminalImage);
#endif
    SerialWriteString(NOMADIX_STAGE_PREFIX " LOAD=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        return Status;
    }

#ifdef NOMADIX_LANNER_LOAD_ONLY
    SerialWriteString(NOMADIX_STAGE_PREFIX " LOAD-ONLY-COMPLETE\r\n");
    return EFI_SUCCESS;
#else
    StartImage = (EFI_START_IMAGE)mBootServices->StartImage;
    ExitDataSize = 0;
    ExitData = 0;
    Status = StartImage(*TerminalImageHandle, &ExitDataSize, &ExitData);
    SerialWriteString(NOMADIX_STAGE_PREFIX " IMAGE_START=");
    SerialWriteHex64(Status);
    SerialWriteString(" EXIT_SIZE=");
    SerialWriteHex64(ExitDataSize);
    SerialWriteString("\r\n");
    if (ExitData != 0) {
        mBootServices->FreePool(ExitData);
    }
    return Status;
#endif
}


static EFI_STATUS StartNewLannerBinding(
    EFI_HANDLE *BeforeHandles,
    UINTN BeforeCount
)
{
    EFI_DRIVER_BINDING_PROTOCOL *Binding;
    EFI_HANDLE *AfterHandles;
    EFI_STATUS Status;
    UINTN AfterCount;
    UINTN Index;
    UINTN Matches;

    AfterHandles = 0;
    AfterCount = 0;
    Matches = 0;
    Status = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mDriverBindingProtocolGuid,
        0,
        &AfterCount,
        &AfterHandles
    );
    SerialWriteString(NOMADIX_STAGE_PREFIX " BINDINGS=");
    SerialWriteHex64(Status);
    SerialWriteString(" BEFORE=");
    SerialWriteHex64(BeforeCount);
    SerialWriteString(" AFTER=");
    SerialWriteHex64(AfterCount);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        return Status;
    }

    Status = EFI_NOT_FOUND;
    for (Index = 0; Index < AfterCount; ++Index) {
        if (HandlesContain(BeforeHandles, BeforeCount, AfterHandles[Index])) {
            continue;
        }
        Binding = 0;
        Status = mBootServices->HandleProtocol(
            AfterHandles[Index],
            &mDriverBindingProtocolGuid,
            (void **)&Binding
        );
        if (Status != EFI_SUCCESS || Binding == 0) {
            continue;
        }
        ++Matches;
        SerialWriteString(NOMADIX_STAGE_PREFIX " CANDIDATE VERSION=");
        SerialWriteHex64(Binding->Version);
        SerialWriteString("\r\n");
        Status = Binding->Supported(Binding, mCom1Handle, 0);
        SerialWriteString(NOMADIX_STAGE_PREFIX " SUPPORTED=");
        SerialWriteHex64(Status);
        SerialWriteString("\r\n");
        if (Status == EFI_SUCCESS) {
            Status = Binding->Start(Binding, mCom1Handle, 0);
            SerialWriteString(NOMADIX_STAGE_PREFIX " DRIVER_START=");
            SerialWriteHex64(Status);
            SerialWriteString("\r\n");
        }
        break;
    }
    mBootServices->FreePool(AfterHandles);
    SerialWriteString(NOMADIX_STAGE_PREFIX " MATCHES=");
    SerialWriteHex64(Matches);
    SerialWriteString("\r\n");
    return Status;
}


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_HANDLE *BeforeHandles;
    EFI_HANDLE TerminalImageHandle;
    EFI_STATUS Status;
    UINTN BeforeCount;

    Status = Stage4F4EfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    SerialWriteString(NOMADIX_STAGE_PREFIX " ARMED\r\n");

    Status = ConvertCom1DevicePathToAcpi();
    if (Status != EFI_SUCCESS) {
        SerialWriteString(NOMADIX_STAGE_PREFIX " PATH-CONVERSION-FAILED\r\n");
        return EFI_SUCCESS;
    }

    BeforeHandles = 0;
    BeforeCount = 0;
    Status = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mDriverBindingProtocolGuid,
        0,
        &BeforeCount,
        &BeforeHandles
    );
    if (Status != EFI_SUCCESS) {
        SerialWriteString(NOMADIX_STAGE_PREFIX " BEFORE-BINDINGS-FAILED\r\n");
        return EFI_SUCCESS;
    }

    TerminalImageHandle = 0;
    Status = LoadLannerTerminal(ImageHandle, &TerminalImageHandle);
    if (Status == EFI_SUCCESS) {
#ifdef NOMADIX_LANNER_LOAD_ONLY
        SerialWriteString(NOMADIX_STAGE_PREFIX " TEST-COMPLETE\r\n");
#else
        Status = StartNewLannerBinding(BeforeHandles, BeforeCount);
#endif
    }
    mBootServices->FreePool(BeforeHandles);
    if (Status != EFI_SUCCESS) {
        SerialWriteString(
            NOMADIX_STAGE_PREFIX " LANNER-TERMINAL-TEST-FAILED\r\n"
        );
    }
    return EFI_SUCCESS;
}
#undef EfiMain
