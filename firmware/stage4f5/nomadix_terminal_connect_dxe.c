#define NOMADIX_STAGE4F4_ENTRY Stage4F4EfiMain
#include "../stage4f4/nomadix_serial_io_dxe.c"
#undef NOMADIX_STAGE4F4_ENTRY


typedef EFI_STATUS (EFIAPI *EFI_CONNECT_CONTROLLER)(
    EFI_HANDLE ControllerHandle,
    EFI_HANDLE *DriverImageHandle,
    void *RemainingDevicePath,
    uint8_t Recursive
);

static uint8_t mConnectAttempts;


static void DumpPostConnectTextHandles(void)
{
    EFI_HANDLE *Handles;
    EFI_STATUS Status;
    UINTN Count;
    UINTN Index;

    Handles = 0;
    Count = 0;
    Status = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mSimpleTextOutProtocolGuid,
        0,
        &Count,
        &Handles
    );
    SerialWriteString("POST_CONNECT TEXT_OUT LOCATE=");
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
        SerialWriteString("POST_TEXT[");
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


static void EFIAPI ConnectCom1Terminal(EFI_EVENT Event, void *Context)
{
    EFI_CONNECT_CONTROLLER ConnectController;
    EFI_STALL Stall;
    EFI_STATUS Status;

    (void)Event;
    (void)Context;
    if (mConnectAttempts >= 2U) {
        return;
    }
    ++mConnectAttempts;
    ConnectController = (EFI_CONNECT_CONTROLLER)mBootServices->ConnectController;
    Status = ConnectController(mCom1Handle, 0, 0, 1U);

    Stall = (EFI_STALL)mBootServices->Stall;
    Stall(100000U);
    SerialConfigure();
    SerialWriteString("\r\n[NOMADIX-4F5] CONNECT=");
    SerialWriteHex64(Status);
    SerialWriteString(" ATTEMPT=");
    SerialWriteHex64(mConnectAttempts);
    SerialWriteString("\r\n");
    DumpPostConnectTextHandles();
    if (Status == EFI_SUCCESS) {
        mConnectAttempts = 2U;
    }
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

    Status = Stage4F4EfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }

    ReadyToBootEvent = 0;
    ReadyStatus = mBootServices->CreateEvent(
        EVT_SIGNAL_READY_TO_BOOT,
        TPL_CALLBACK,
        ConnectCom1Terminal,
        0,
        &ReadyToBootEvent
    );
    LegacyBootEvent = 0;
    LegacyStatus = mBootServices->CreateEvent(
        EVT_SIGNAL_LEGACY_BOOT,
        TPL_CALLBACK,
        ConnectCom1Terminal,
        0,
        &LegacyBootEvent
    );
    if (ReadyStatus != EFI_SUCCESS && LegacyStatus != EFI_SUCCESS) {
        return EFI_DEVICE_ERROR;
    }
    return EFI_SUCCESS;
}
