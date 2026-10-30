#define NOMADIX_STAGE4GN_ENTRY Stage4GNBrokenEfiMain
#include "../stage4gn/nomadix_single_port_pxe_dxe.c"
#undef NOMADIX_STAGE4GN_ENTRY


typedef EFI_STATUS (EFIAPI *EFI_REGISTER_PROTOCOL_NOTIFY)(
    const EFI_GUID *Protocol,
    EFI_EVENT Event,
    void **Registration
);

static EFI_EVENT mDriverBindingNotifyEvent;
static void *mDriverBindingRegistration;
static uint8_t mPrimaryNetworkTimerArmed;


static EFI_STATUS ArmPrimaryNetworkTimer(void)
{
    EFI_SET_TIMER SetTimer;
    EFI_STATUS Status;

    if (mPrimaryNetworkTimerArmed != 0U) {
        return EFI_SUCCESS;
    }
    SetTimer = (EFI_SET_TIMER)mBootServices->SetTimer;
    Status = SetTimer(
        mNetworkRetryEvent,
        TIMER_PERIODIC,
        NETWORK_RETRY_INTERVAL_100NS
    );
    if (Status == EFI_SUCCESS) {
        mPrimaryNetworkTimerArmed = 1U;
        NetworkSerialWriteString("[NOMADIX-4GO] FILTERED-TIMER-ARMED\r\n");
    }
    return Status;
}


static void EFIAPI InstallFilterWhenIntelAppears(
    EFI_EVENT Event,
    void *Context
)
{
    EFI_STATUS Status;

    (void)Event;
    (void)Context;
    if (mIntelBinding != 0) {
        return;
    }
    Status = InstallSinglePortFilter();
    if (Status != EFI_SUCCESS) {
        return;
    }
    NetworkSerialWriteString("[NOMADIX-4GO] FILTER=0000000000000000\r\n");
    ArmPrimaryNetworkTimer();
}


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_CLOSE_EVENT CloseEvent;
    EFI_REGISTER_PROTOCOL_NOTIFY RegisterProtocolNotify;
    EFI_SET_TIMER SetTimer;
    EFI_STATUS Status;

    Status = Stage4GNBaseEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    SetTimer = (EFI_SET_TIMER)mBootServices->SetTimer;
    SetTimer(mNetworkRetryEvent, TIMER_CANCEL, 0);
    CloseEvent = (EFI_CLOSE_EVENT)mBootServices->CloseEvent;
    Status = CloseEvent(mNetworkRetryEvent);
    if (Status != EFI_SUCCESS) {
        return Status;
    }

    mNetworkRetryEvent = 0;
    mConnectedNetworkHandleCount = 0;
    mNetworkRetryCount = 0;
    mPrimaryNicHandle = 0;
    mIntelHiiRemoved = 0;
    mIntelBinding = 0;
    mOriginalIntelSupported = 0;
    mPrimaryNetworkTimerArmed = 0;
    Status = mBootServices->CreateEvent(
        EVT_TIMER | EVT_NOTIFY_SIGNAL,
        TPL_CALLBACK,
        ConnectPrimaryNetworkStack,
        0,
        &mNetworkRetryEvent
    );
    if (Status != EFI_SUCCESS) {
        return Status;
    }

    mDriverBindingNotifyEvent = 0;
    mDriverBindingRegistration = 0;
    Status = mBootServices->CreateEvent(
        EVT_NOTIFY_SIGNAL,
        TPL_CALLBACK,
        InstallFilterWhenIntelAppears,
        0,
        &mDriverBindingNotifyEvent
    );
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    RegisterProtocolNotify =
        (EFI_REGISTER_PROTOCOL_NOTIFY)mBootServices->RegisterProtocolNotify;
    Status = RegisterProtocolNotify(
        &mDriverBindingProtocolGuidSinglePort,
        mDriverBindingNotifyEvent,
        &mDriverBindingRegistration
    );
    if (Status != EFI_SUCCESS) {
        return Status;
    }

    InstallFilterWhenIntelAppears(mDriverBindingNotifyEvent, 0);
    NetworkSerialWriteString("[NOMADIX-4GO] FILTER-WAIT-ARMED\r\n");
    return EFI_SUCCESS;
}
