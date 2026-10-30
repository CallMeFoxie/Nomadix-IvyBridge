#ifndef NOMADIX_STAGE4F8_ENTRY
#define NOMADIX_STAGE4F8_ENTRY EfiMain
#endif
#define NOMADIX_STAGE4F7_ENTRY Stage4F7EfiMain
#include "../stage4f7/nomadix_terminal_direct_start_dxe.c"
#undef NOMADIX_STAGE4F7_ENTRY
#define EfiMain NOMADIX_STAGE4F8_ENTRY


#define TERMINAL_PRIVATE_CHECK_OFFSET 0x2D28U
#define TERMINAL_EXPECTED_SIZE 0xC440U

#ifndef NOMADIX_AFTER_TERMINAL_START
#define NOMADIX_AFTER_TERMINAL_START(Binding, Status) ((void)0)
#endif
#ifndef NOMADIX_PATCH_TERMINAL_IMAGE
#define NOMADIX_PATCH_TERMINAL_IMAGE(Image, Size) EFI_SUCCESS
#endif

static const uint8_t mExpectedPrivateCheck[8] = {
    0x84, 0xC0, 0x0F, 0x85, 0x8E, 0x05, 0x00, 0x00
};


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


static EFI_STATUS LoadPatchedTerminal(
    EFI_HANDLE ParentImageHandle,
    EFI_HANDLE *PatchedImageHandle
)
{
    EFI_FIRMWARE_VOLUME2_PROTOCOL *FirmwareVolume;
    EFI_HANDLE *Handles;
    EFI_LOAD_IMAGE LoadImage;
    EFI_START_IMAGE StartImage;
    EFI_STATUS ReadStatus;
    EFI_STATUS Status;
    UINTN Count;
    UINTN ExitDataSize;
    UINTN Index;
    uint16_t *ExitData;
    uint8_t *TerminalImage;
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
            (void **)&TerminalImage,
            &TerminalImageSize,
            &AuthenticationStatus
        );
        if (ReadStatus == EFI_SUCCESS) {
            break;
        }
    }
    mBootServices->FreePool(Handles);
    SerialWriteString("[NOMADIX-4F8] READ=");
    SerialWriteHex64(ReadStatus);
    SerialWriteString(" SIZE=");
    SerialWriteHex64(TerminalImageSize);
    SerialWriteString("\r\n");
    if (ReadStatus != EFI_SUCCESS) {
        return ReadStatus;
    }
    if (TerminalImageSize != TERMINAL_EXPECTED_SIZE) {
        mBootServices->FreePool(TerminalImage);
        return EFI_UNSUPPORTED;
    }
    for (Index = 0; Index < sizeof(mExpectedPrivateCheck); ++Index) {
        if (TerminalImage[TERMINAL_PRIVATE_CHECK_OFFSET + Index] !=
            mExpectedPrivateCheck[Index]) {
            mBootServices->FreePool(TerminalImage);
            return EFI_UNSUPPORTED;
        }
    }
    for (Index = 0; Index < sizeof(mExpectedPrivateCheck); ++Index) {
        TerminalImage[TERMINAL_PRIVATE_CHECK_OFFSET + Index] = 0x90U;
    }
    Status = NOMADIX_PATCH_TERMINAL_IMAGE(
        TerminalImage,
        TerminalImageSize
    );
    if (Status != EFI_SUCCESS) {
        mBootServices->FreePool(TerminalImage);
        return Status;
    }
    SerialWriteString("[NOMADIX-4F8] PATCH=0000000000000000\r\n");

    LoadImage = (EFI_LOAD_IMAGE)mBootServices->LoadImage;
    *PatchedImageHandle = 0;
    Status = LoadImage(
        0,
        ParentImageHandle,
        0,
        TerminalImage,
        TerminalImageSize,
        PatchedImageHandle
    );
    mBootServices->FreePool(TerminalImage);
    SerialWriteString("[NOMADIX-4F8] LOAD=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        return Status;
    }

    StartImage = (EFI_START_IMAGE)mBootServices->StartImage;
    ExitDataSize = 0;
    ExitData = 0;
    Status = StartImage(*PatchedImageHandle, &ExitDataSize, &ExitData);
    SerialWriteString("[NOMADIX-4F8] IMAGE_START=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    if (ExitData != 0) {
        mBootServices->FreePool(ExitData);
    }
    return Status;
}


static EFI_STATUS StartNewTerminalBinding(
    EFI_HANDLE *BeforeHandles,
    UINTN BeforeCount
)
{
    EFI_DRIVER_BINDING_PROTOCOL *Binding;
    EFI_HANDLE *AfterHandles;
    EFI_STATUS Status;
    UINTN AfterCount;
    UINTN Index;

    AfterHandles = 0;
    AfterCount = 0;
    Status = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mDriverBindingProtocolGuid,
        0,
        &AfterCount,
        &AfterHandles
    );
    SerialWriteString("[NOMADIX-4F8] BINDINGS=");
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
        Status = Binding->Supported(Binding, mCom1Handle, 0);
        SerialWriteString("[NOMADIX-4F8] SUPPORTED=");
        SerialWriteHex64(Status);
        SerialWriteString("\r\n");
        if (Status == EFI_SUCCESS) {
            Status = Binding->Start(Binding, mCom1Handle, 0);
            NOMADIX_AFTER_TERMINAL_START(Binding, Status);
            SerialWriteString("[NOMADIX-4F8] DRIVER_START=");
            SerialWriteHex64(Status);
            SerialWriteString("\r\n");
        }
        break;
    }
    mBootServices->FreePool(AfterHandles);
    return Status;
}


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_HANDLE *BeforeHandles;
    EFI_HANDLE PatchedImageHandle;
    EFI_STATUS Status;
    UINTN BeforeCount;

    Status = Stage4F7EfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
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
        return EFI_SUCCESS;
    }

    PatchedImageHandle = 0;
    Status = LoadPatchedTerminal(ImageHandle, &PatchedImageHandle);
    if (Status == EFI_SUCCESS) {
        Status = StartNewTerminalBinding(BeforeHandles, BeforeCount);
    }
    mBootServices->FreePool(BeforeHandles);
    if (Status != EFI_SUCCESS) {
        SerialWriteString("[NOMADIX-4F8] PRIVATE-BYPASS-DIAG-FAILED\r\n");
    }
    return EFI_SUCCESS;
}
#undef EfiMain
