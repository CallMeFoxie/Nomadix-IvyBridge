#define NOMADIX_STAGE4GG_ENTRY Stage4GJBaseEfiMain
#include "../stage4gh/nomadix_quiet_final_dxe.c"
#undef NOMADIX_STAGE4GG_ENTRY


typedef EFI_STATUS (EFIAPI *EFI_REGISTER_PROTOCOL_NOTIFY)(
    const EFI_GUID *Protocol,
    EFI_EVENT Event,
    void **Registration
);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE)(
    uint32_t SearchType,
    const EFI_GUID *Protocol,
    void *SearchKey,
    UINTN *BufferSize,
    EFI_HANDLE *Buffer
);
typedef EFI_STATUS (EFIAPI *EFI_CONNECT_CONTROLLER)(
    EFI_HANDLE ControllerHandle,
    EFI_HANDLE *DriverImageHandle,
    void *RemainingDevicePath,
    uint8_t Recursive
);

#define EFI_BY_REGISTER_NOTIFY 1U
#define EVT_NOTIFY_SIGNAL 0x00000200U

static const EFI_GUID mNetworkInterfaceIdentifierProtocolGuid31 = {
    0x1ACED566,
    0x76ED,
    0x4218,
    { 0xBC, 0x81, 0x76, 0x7F, 0x1F, 0x97, 0x7A, 0x89 }
};
static EFI_EVENT mNetworkInterfaceNotifyEvent;
static void *mNetworkInterfaceRegistration;


static void EFIAPI ConnectNewNetworkInterfaces(
    EFI_EVENT Event,
    void *Context
)
{
    EFI_CONNECT_CONTROLLER ConnectController;
    EFI_LOCATE_HANDLE LocateHandle;

    (void)Event;
    (void)Context;
    if (mNetworkInterfaceRegistration == 0) {
        return;
    }
    LocateHandle = (EFI_LOCATE_HANDLE)mBootServices->LocateHandle;
    ConnectController =
        (EFI_CONNECT_CONTROLLER)mBootServices->ConnectController;
    for (;;) {
        EFI_HANDLE Handle;
        EFI_STATUS ConnectStatus;
        EFI_STATUS LocateStatus;
        UINTN BufferSize;

        Handle = 0;
        BufferSize = sizeof(Handle);
        LocateStatus = LocateHandle(
            EFI_BY_REGISTER_NOTIFY,
            0,
            mNetworkInterfaceRegistration,
            &BufferSize,
            &Handle
        );
        if (LocateStatus != EFI_SUCCESS) {
            break;
        }
        ConnectStatus = ConnectController(Handle, 0, 0, 1U);
        SerialWriteString("[NOMADIX-4GJ] NII=");
        SerialWriteHex64((UINTN)Handle);
        SerialWriteString(" CONNECT=");
        SerialWriteHex64(ConnectStatus);
        SerialWriteString("\r\n");
    }
}


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_REGISTER_PROTOCOL_NOTIFY RegisterProtocolNotify;
    EFI_STATUS Status;

    Status = Stage4GJBaseEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    mNetworkInterfaceNotifyEvent = 0;
    mNetworkInterfaceRegistration = 0;
    Status = mBootServices->CreateEvent(
        EVT_NOTIFY_SIGNAL,
        TPL_CALLBACK,
        ConnectNewNetworkInterfaces,
        0,
        &mNetworkInterfaceNotifyEvent
    );
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    RegisterProtocolNotify =
        (EFI_REGISTER_PROTOCOL_NOTIFY)mBootServices->RegisterProtocolNotify;
    Status = RegisterProtocolNotify(
        &mNetworkInterfaceIdentifierProtocolGuid31,
        mNetworkInterfaceNotifyEvent,
        &mNetworkInterfaceRegistration
    );
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    ConnectNewNetworkInterfaces(mNetworkInterfaceNotifyEvent, 0);
    return EFI_SUCCESS;
}
