#ifndef NOMADIX_STAGE4GL_ENTRY
#define NOMADIX_STAGE4GL_ENTRY EfiMain
#endif
#define NOMADIX_STAGE4GK_ENTRY Stage4GLBaseEfiMain
#include "../stage4gk/nomadix_deferred_network_reconnect_dxe.c"
#undef NOMADIX_STAGE4GK_ENTRY


static const EFI_GUID mNiiPointerProtocolGuid = {
    0xE3161450,
    0xAD0F,
    0x11D9,
    { 0x96, 0x69, 0x08, 0x00, 0x20, 0x0C, 0x9A, 0x66 }
};


static void EFIAPI ConnectNiiPointerHandles(
    EFI_EVENT Event,
    void *Context
)
{
    EFI_CONNECT_CONTROLLER ConnectController;
    EFI_HANDLE *Handles;
    EFI_SET_TIMER SetTimer;
    EFI_STATUS LocateStatus;
    UINTN Count;
    UINTN Index;

    (void)Context;
    ++mNetworkRetryCount;
    Handles = 0;
    Count = 0;
    LocateStatus = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mNiiPointerProtocolGuid,
        0,
        &Count,
        &Handles
    );
    if (LocateStatus == EFI_SUCCESS) {
        ConnectController =
            (EFI_CONNECT_CONTROLLER)mBootServices->ConnectController;
        for (Index = 0; Index < Count; ++Index) {
            EFI_STATUS ConnectStatus;
            EFI_STATUS SnpStatus;
            void *SimpleNetwork;

            if (IsConnectedNetworkHandle(Handles[Index]) != 0U) {
                continue;
            }
            ConnectStatus = ConnectController(Handles[Index], 0, 0, 1U);
            SimpleNetwork = 0;
            SnpStatus = mBootServices->HandleProtocol(
                Handles[Index],
                &mSimpleNetworkProtocolGuid,
                &SimpleNetwork
            );
            if (SnpStatus == EFI_SUCCESS && SimpleNetwork != 0) {
                RememberConnectedNetworkHandle(Handles[Index]);
                NetworkSerialWriteString("[NOMADIX-4GL] SNP-HANDLE=");
                NetworkSerialWriteHex64((UINTN)Handles[Index]);
                NetworkSerialWriteString(" CONNECT=");
                NetworkSerialWriteHex64(ConnectStatus);
                NetworkSerialWriteString("\r\n");
            }
        }
        mBootServices->FreePool(Handles);
    }
    if (
        mConnectedNetworkHandleCount >= 6U ||
        mNetworkRetryCount >= NETWORK_RETRY_LIMIT
    ) {
        SetTimer = (EFI_SET_TIMER)mBootServices->SetTimer;
        SetTimer(Event, TIMER_CANCEL, 0);
        NetworkSerialWriteString("[NOMADIX-4GL] COMPLETE COUNT=");
        NetworkSerialWriteHex64(mConnectedNetworkHandleCount);
        NetworkSerialWriteString(" RETRIES=");
        NetworkSerialWriteHex64(mNetworkRetryCount);
        NetworkSerialWriteString("\r\n");
    }
}


EFI_STATUS EFIAPI NOMADIX_STAGE4GL_ENTRY(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_CLOSE_EVENT CloseEvent;
    EFI_SET_TIMER SetTimer;
    EFI_STATUS Status;

    Status = Stage4GLBaseEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    SetTimer = (EFI_SET_TIMER)mBootServices->SetTimer;
    SetTimer(mNetworkRetryEvent, TIMER_CANCEL, 0);
    mConnectedNetworkHandleCount = 0;
    mNetworkRetryCount = 0;
    CloseEvent = (EFI_CLOSE_EVENT)mBootServices->CloseEvent;
    Status = CloseEvent(mNetworkRetryEvent);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    mNetworkRetryEvent = 0;
    Status = mBootServices->CreateEvent(
        EVT_TIMER | EVT_NOTIFY_SIGNAL,
        TPL_CALLBACK,
        ConnectNiiPointerHandles,
        0,
        &mNetworkRetryEvent
    );
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = SetTimer(
        mNetworkRetryEvent,
        TIMER_PERIODIC,
        NETWORK_RETRY_INTERVAL_100NS
    );
    if (Status == EFI_SUCCESS) {
        NetworkSerialWriteString("[NOMADIX-4GL] TIMER-ARMED\r\n");
    }
    return Status;
}
