#include <stdint.h>

static uint64_t DirectConnectOndaTerminal(void *RemainingDevicePath);

#ifndef NOMADIX_PREPARE_ONDA_SERIAL_VARIABLES
#define NOMADIX_PREPARE_ONDA_SERIAL_VARIABLES() EFI_SUCCESS
#endif
#ifndef NOMADIX_RESTORE_ONDA_SERIAL_VARIABLES
#define NOMADIX_RESTORE_ONDA_SERIAL_VARIABLES() ((void)0)
#endif
#ifndef NOMADIX_STAGE_PREFIX
#define NOMADIX_STAGE_PREFIX "[NOMADIX-4FI]"
#endif
#define NOMADIX_CUSTOM_CONNECT_ONDA_TERMINAL
#define NOMADIX_CONNECT_ONDA_TERMINAL(Status, RemainingDevicePath) \
    ((Status) = (EFI_STATUS)DirectConnectOndaTerminal(RemainingDevicePath))
#include "../stage4fh/nomadix_onda_terminal_backend_connect_dxe.c"

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

static const EFI_GUID mDirectDriverBindingProtocolGuid = {
    0x18A031AB,
    0xB443,
    0x4D1A,
    {0xA5, 0xC0, 0x0C, 0x09, 0x26, 0x1E, 0x9F, 0x71}
};


static uint64_t DirectConnectOndaTerminal(void *RemainingDevicePath)
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
    Status = NOMADIX_PREPARE_ONDA_SERIAL_VARIABLES();
    SerialWriteString(NOMADIX_STAGE_PREFIX " SERIAL-VARS=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        NOMADIX_RESTORE_ONDA_SERIAL_VARIABLES();
        return Status;
    }
    Status = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mDirectDriverBindingProtocolGuid,
        0,
        &Count,
        &Handles
    );
    SerialWriteString(NOMADIX_STAGE_PREFIX " BINDINGS=");
    SerialWriteHex64(Status);
    SerialWriteString(" COUNT=");
    SerialWriteHex64(Count);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        NOMADIX_RESTORE_ONDA_SERIAL_VARIABLES();
        return Status;
    }

    StartStatus = EFIERR(14);
    for (Index = 0; Index < Count; ++Index) {
        Binding = 0;
        Status = mBootServices->HandleProtocol(
            Handles[Index],
            &mDirectDriverBindingProtocolGuid,
            (void **)&Binding
        );
        if (Status != EFI_SUCCESS || Binding == 0) {
            continue;
        }
        Status = Binding->Supported(
            Binding,
            mCom1Handle,
            RemainingDevicePath
        );
        if (Status != EFI_UNSUPPORTED) {
            SerialWriteString(NOMADIX_STAGE_PREFIX " CANDIDATE INDEX=");
            SerialWriteHex64(Index);
            SerialWriteString(" VERSION=");
            SerialWriteHex64(Binding->Version);
            SerialWriteString(" SUPPORTED=");
            SerialWriteHex64(Status);
            SerialWriteString("\r\n");
        }
        if (Status == EFI_SUCCESS) {
            ++Matches;
            StartStatus = Binding->Start(
                Binding,
                mCom1Handle,
                RemainingDevicePath
            );
            SerialWriteString(NOMADIX_STAGE_PREFIX " START=");
            SerialWriteHex64(StartStatus);
            SerialWriteString("\r\n");
            break;
        }
    }
    mBootServices->FreePool(Handles);
    NOMADIX_RESTORE_ONDA_SERIAL_VARIABLES();
    SerialWriteString(NOMADIX_STAGE_PREFIX " MATCHES=");
    SerialWriteHex64(Matches);
    SerialWriteString("\r\n");
    return StartStatus;
}
