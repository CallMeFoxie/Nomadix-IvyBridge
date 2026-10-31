#include <stdint.h>

#ifndef NOMADIX_STAGE4FA_ENTRY
#define NOMADIX_STAGE4FA_ENTRY EfiMain
#endif

static void TraceDevicePathOpen(void *BindingPointer, uint64_t StartStatus);

#define NOMADIX_AFTER_TERMINAL_START(Binding, Status) \
    TraceDevicePathOpen((void *)(Binding), (uint64_t)(Status))
#define NOMADIX_STAGE4F9_ENTRY Stage4F9EfiMain
#include "../stage4f9/nomadix_terminal_serial_call_trace_dxe.c"
#undef NOMADIX_STAGE4F9_ENTRY
#define EfiMain NOMADIX_STAGE4FA_ENTRY


#define EFI_OPEN_PROTOCOL_BY_DRIVER 0x00000002U

typedef EFI_STATUS (EFIAPI *EFI_OPEN_PROTOCOL)(
    EFI_HANDLE Handle,
    const EFI_GUID *Protocol,
    void **Interface,
    EFI_HANDLE AgentHandle,
    EFI_HANDLE ControllerHandle,
    uint32_t Attributes
);
typedef EFI_STATUS (EFIAPI *EFI_CLOSE_PROTOCOL)(
    EFI_HANDLE Handle,
    const EFI_GUID *Protocol,
    EFI_HANDLE AgentHandle,
    EFI_HANDLE ControllerHandle
);

typedef struct {
    EFI_HANDLE AgentHandle;
    EFI_HANDLE ControllerHandle;
    uint32_t Attributes;
    uint32_t OpenCount;
} EFI_OPEN_PROTOCOL_INFORMATION_ENTRY;

typedef EFI_STATUS (EFIAPI *EFI_OPEN_PROTOCOL_INFORMATION)(
    EFI_HANDLE Handle,
    const EFI_GUID *Protocol,
    EFI_OPEN_PROTOCOL_INFORMATION_ENTRY **EntryBuffer,
    UINTN *EntryCount
);


static void TraceDevicePathOpen(void *BindingPointer, uint64_t StartStatus)
{
    EFI_DRIVER_BINDING_PROTOCOL *Binding;
    EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *Entries;
    EFI_CLOSE_PROTOCOL CloseProtocol;
    EFI_OPEN_PROTOCOL_INFORMATION OpenProtocolInformation;
    EFI_OPEN_PROTOCOL OpenProtocol;
    EFI_STATUS InformationStatus;
    EFI_STATUS ReopenStatus;
    UINTN Count;
    UINTN Index;
    void *Interface;

    Binding = (EFI_DRIVER_BINDING_PROTOCOL *)BindingPointer;
    SerialWriteString("[NOMADIX-4FA] START_STATUS=");
    SerialWriteHex64(StartStatus);
    SerialWriteString("\r\n");
    if (Binding == 0 || mCom1Handle == 0) {
        return;
    }

    Entries = 0;
    Count = 0;
    OpenProtocolInformation =
        (EFI_OPEN_PROTOCOL_INFORMATION)mBootServices->OpenProtocolInformation;
    InformationStatus = OpenProtocolInformation(
        mCom1Handle,
        &mDevicePathProtocolGuid,
        &Entries,
        &Count
    );
    SerialWriteString("[NOMADIX-4FA] DP_OPEN_INFO=");
    SerialWriteHex64(InformationStatus);
    SerialWriteString(" COUNT=");
    SerialWriteHex64(Count);
    SerialWriteString("\r\n");
    if (InformationStatus == EFI_SUCCESS) {
        for (Index = 0; Index < Count; ++Index) {
            SerialWriteString("[NOMADIX-4FA] DP_OPEN INDEX=");
            SerialWriteHex64(Index);
            SerialWriteString(" AGENT=");
            SerialWriteHex64((uint64_t)(UINTN)Entries[Index].AgentHandle);
            SerialWriteString(" CONTROLLER=");
            SerialWriteHex64(
                (uint64_t)(UINTN)Entries[Index].ControllerHandle
            );
            SerialWriteString(" ATTR=");
            SerialWriteHex64(Entries[Index].Attributes);
            SerialWriteString(" OPEN_COUNT=");
            SerialWriteHex64(Entries[Index].OpenCount);
            SerialWriteString("\r\n");
        }
        if (Entries != 0) {
            mBootServices->FreePool(Entries);
        }
    }

    Interface = 0;
    OpenProtocol = (EFI_OPEN_PROTOCOL)mBootServices->OpenProtocol;
    ReopenStatus = OpenProtocol(
        mCom1Handle,
        &mDevicePathProtocolGuid,
        &Interface,
        Binding->DriverBindingHandle,
        mCom1Handle,
        EFI_OPEN_PROTOCOL_BY_DRIVER
    );
    SerialWriteString("[NOMADIX-4FA] DP_REOPEN=");
    SerialWriteHex64(ReopenStatus);
    SerialWriteString(" INTERFACE=");
    SerialWriteHex64((uint64_t)(UINTN)Interface);
    SerialWriteString("\r\n");
    if (ReopenStatus == EFI_SUCCESS) {
        CloseProtocol = (EFI_CLOSE_PROTOCOL)mBootServices->CloseProtocol;
        ReopenStatus = CloseProtocol(
            mCom1Handle,
            &mDevicePathProtocolGuid,
            Binding->DriverBindingHandle,
            mCom1Handle
        );
        SerialWriteString("[NOMADIX-4FA] DP_REOPEN_CLOSE=");
        SerialWriteHex64(ReopenStatus);
        SerialWriteString("\r\n");
    }
}


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    SerialWriteString("[NOMADIX-4FA] ARMED\r\n");
    return Stage4F9EfiMain(ImageHandle, SystemTable);
}
#undef EfiMain
