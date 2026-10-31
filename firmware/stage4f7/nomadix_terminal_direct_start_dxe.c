#ifndef NOMADIX_STAGE4F7_ENTRY
#define NOMADIX_STAGE4F7_ENTRY EfiMain
#endif
#define NOMADIX_STAGE4F6_ENTRY Stage4F6EfiMain
#include "../stage4f6/nomadix_terminal_reload_dxe.c"
#undef NOMADIX_STAGE4F6_ENTRY
#define EfiMain NOMADIX_STAGE4F7_ENTRY


typedef struct EFI_DRIVER_BINDING_PROTOCOL EFI_DRIVER_BINDING_PROTOCOL;

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
typedef char AssertDriverBindingImageOffset[
    offsetof(EFI_DRIVER_BINDING_PROTOCOL, ImageHandle) == 32 ? 1 : -1
];

static const EFI_GUID mDriverBindingProtocolGuid = {
    0x18A031AB,
    0xB443,
    0x4D1A,
    {0xA5, 0xC0, 0x0C, 0x09, 0x26, 0x1E, 0x9F, 0x71}
};


static EFI_STATUS DirectStartTerminalBinding(void)
{
    EFI_DRIVER_BINDING_PROTOCOL *Binding;
    EFI_HANDLE *Handles;
    EFI_STATUS StartStatus;
    EFI_STATUS Status;
    UINTN Count;
    UINTN Index;
    UINTN Matches;

    Handles = 0;
    Count = 0;
    Matches = 0;
    Status = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mDriverBindingProtocolGuid,
        0,
        &Count,
        &Handles
    );
    SerialWriteString("[NOMADIX-4F7] BINDINGS=");
    SerialWriteHex64(Status);
    SerialWriteString(" COUNT=");
    SerialWriteHex64(Count);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        return Status;
    }

    StartStatus = EFI_NOT_FOUND;
    for (Index = 0; Index < Count; ++Index) {
        Binding = 0;
        Status = mBootServices->HandleProtocol(
            Handles[Index],
            &mDriverBindingProtocolGuid,
            (void **)&Binding
        );
        if (Status != EFI_SUCCESS || Binding == 0) {
            continue;
        }
        Status = Binding->Supported(Binding, mCom1Handle, 0);
        if (Status != EFI_UNSUPPORTED) {
            SerialWriteString("[NOMADIX-4F7] CANDIDATE INDEX=");
            SerialWriteHex64(Index);
            SerialWriteString(" VERSION=");
            SerialWriteHex64(Binding->Version);
            SerialWriteString(" SUPPORTED=");
            SerialWriteHex64(Status);
            SerialWriteString("\r\n");
        }
        if (Status == EFI_SUCCESS) {
            ++Matches;
            StartStatus = Binding->Start(Binding, mCom1Handle, 0);
            SerialWriteString("[NOMADIX-4F7] START=");
            SerialWriteHex64(StartStatus);
            SerialWriteString("\r\n");
            break;
        }
    }
    mBootServices->FreePool(Handles);
    SerialWriteString("[NOMADIX-4F7] MATCHES=");
    SerialWriteHex64(Matches);
    SerialWriteString("\r\n");
    return StartStatus;
}


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;

    Status = Stage4F6EfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = DirectStartTerminalBinding();
    if (Status != EFI_SUCCESS) {
        SerialWriteString("[NOMADIX-4F7] DIRECT-START-DIAG-FAILED\r\n");
    }
    return EFI_SUCCESS;
}
#undef EfiMain
