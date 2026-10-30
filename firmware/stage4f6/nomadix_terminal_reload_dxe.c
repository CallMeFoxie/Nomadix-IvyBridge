#ifndef NOMADIX_STAGE4F6_ENTRY
#define NOMADIX_STAGE4F6_ENTRY EfiMain
#endif
#define NOMADIX_STAGE4F4_ENTRY Stage4F4EfiMain
#include "../stage4f4/nomadix_serial_io_dxe.c"
#undef NOMADIX_STAGE4F4_ENTRY
#define EfiMain NOMADIX_STAGE4F6_ENTRY

#define EFI_NOT_FOUND EFIERR(14)

typedef struct EFI_FIRMWARE_VOLUME2_PROTOCOL
    EFI_FIRMWARE_VOLUME2_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_FV_READ_SECTION)(
    const EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
    const EFI_GUID *NameGuid,
    uint8_t SectionType,
    UINTN SectionInstance,
    void **Buffer,
    UINTN *BufferSize,
    uint32_t *AuthenticationStatus
);
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
typedef EFI_STATUS (EFIAPI *EFI_CONNECT_CONTROLLER)(
    EFI_HANDLE ControllerHandle,
    EFI_HANDLE *DriverImageHandle,
    void *RemainingDevicePath,
    uint8_t Recursive
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

typedef char AssertFvReadSectionOffset[
    offsetof(EFI_FIRMWARE_VOLUME2_PROTOCOL, ReadSection) == 24 ? 1 : -1
];

static const EFI_GUID mFirmwareVolume2ProtocolGuid = {
    0x220E73B6,
    0x6BDB,
    0x4413,
    {0x84, 0x05, 0xB9, 0x74, 0xB1, 0x08, 0x61, 0x9A}
};
static const EFI_GUID mOndaTerminalFileGuid = {
    0x7A08CB98,
    0xE9BC,
    0x41C3,
    {0xBE, 0x19, 0xB3, 0x02, 0xF3, 0xF1, 0xF5, 0x95}
};


static EFI_STATUS ReloadOndaTerminal(EFI_HANDLE ParentImageHandle)
{
    EFI_CONNECT_CONTROLLER ConnectController;
    EFI_FIRMWARE_VOLUME2_PROTOCOL *FirmwareVolume;
    EFI_HANDLE *Handles;
    EFI_HANDLE TerminalImageHandle;
    EFI_LOAD_IMAGE LoadImage;
    EFI_START_IMAGE StartImage;
    EFI_STATUS ReadStatus;
    EFI_STATUS Status;
    UINTN Count;
    UINTN ExitDataSize;
    UINTN Index;
    uint16_t *ExitData;
    void *TerminalImage;
    UINTN TerminalImageSize;
    uint32_t AuthenticationStatus;

    Handles = 0;
    Count = 0;
    Status = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mFirmwareVolume2ProtocolGuid,
        0,
        &Count,
        &Handles
    );
    SerialWriteString("[NOMADIX-4F6] FV_LOCATE=");
    SerialWriteHex64(Status);
    SerialWriteString(" COUNT=");
    SerialWriteHex64(Count);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        return Status;
    }

    ReadStatus = EFI_NOT_FOUND;
    TerminalImage = 0;
    TerminalImageSize = 0;
    AuthenticationStatus = 0;
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
            &mOndaTerminalFileGuid,
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
    SerialWriteString("[NOMADIX-4F6] READ=");
    SerialWriteHex64(ReadStatus);
    SerialWriteString(" SIZE=");
    SerialWriteHex64(TerminalImageSize);
    SerialWriteString(" AUTH=");
    SerialWriteHex64(AuthenticationStatus);
    SerialWriteString("\r\n");
    if (ReadStatus != EFI_SUCCESS) {
        return ReadStatus;
    }

    LoadImage = (EFI_LOAD_IMAGE)mBootServices->LoadImage;
    TerminalImageHandle = 0;
    Status = LoadImage(
        0,
        ParentImageHandle,
        0,
        TerminalImage,
        TerminalImageSize,
        &TerminalImageHandle
    );
    mBootServices->FreePool(TerminalImage);
    SerialWriteString("[NOMADIX-4F6] LOAD=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        return Status;
    }

    StartImage = (EFI_START_IMAGE)mBootServices->StartImage;
    ExitDataSize = 0;
    ExitData = 0;
    Status = StartImage(TerminalImageHandle, &ExitDataSize, &ExitData);
    SerialWriteString("[NOMADIX-4F6] START=");
    SerialWriteHex64(Status);
    SerialWriteString(" EXIT_SIZE=");
    SerialWriteHex64(ExitDataSize);
    SerialWriteString("\r\n");
    if (ExitData != 0) {
        mBootServices->FreePool(ExitData);
    }
    if (Status != EFI_SUCCESS) {
        return Status;
    }

    ConnectController =
        (EFI_CONNECT_CONTROLLER)mBootServices->ConnectController;
    Status = ConnectController(mCom1Handle, 0, 0, 1U);
    SerialWriteString("[NOMADIX-4F6] CONNECT=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    return Status;
}


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;

    Status = Stage4F4EfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = ReloadOndaTerminal(ImageHandle);
    if (Status != EFI_SUCCESS) {
        SerialWriteString("[NOMADIX-4F6] RELOAD-DIAG-FAILED\r\n");
    }
    return EFI_SUCCESS;
}
#undef EfiMain
