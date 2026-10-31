#ifndef NOMADIX_STAGE4GK_ENTRY
#define NOMADIX_STAGE4GK_ENTRY EfiMain
#endif
#define NOMADIX_STAGE4GG_ENTRY Stage4GKBaseEfiMain
#include "../stage4gh/nomadix_quiet_final_dxe.c"
#undef NOMADIX_STAGE4GG_ENTRY


typedef EFI_STATUS (EFIAPI *EFI_SET_TIMER)(
    EFI_EVENT Event,
    uint32_t Type,
    uint64_t TriggerTime
);
typedef EFI_STATUS (EFIAPI *EFI_CONNECT_CONTROLLER)(
    EFI_HANDLE ControllerHandle,
    EFI_HANDLE *DriverImageHandle,
    void *RemainingDevicePath,
    uint8_t Recursive
);

#define EVT_TIMER 0x80000000U
#define EVT_NOTIFY_SIGNAL 0x00000200U
#define TIMER_CANCEL 0U
#define TIMER_PERIODIC 1U
#define NETWORK_RETRY_INTERVAL_100NS 100000ULL
#define NETWORK_RETRY_LIMIT 200U
#define NETWORK_HANDLE_LIMIT 16U
#ifndef NOMADIX_ENABLE_NETWORK_DIAGNOSTICS
#define NOMADIX_ENABLE_NETWORK_DIAGNOSTICS 1
#endif

static const EFI_GUID mNetworkInterfaceIdentifierProtocolGuid31 = {
    0x1ACED566,
    0x76ED,
    0x4218,
    { 0xBC, 0x81, 0x76, 0x7F, 0x1F, 0x97, 0x7A, 0x89 }
};
static const EFI_GUID mSimpleNetworkProtocolGuid = {
    0xA19832B9,
    0xAC25,
    0x11D3,
    { 0x9A, 0x2D, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D }
};
static EFI_EVENT mNetworkRetryEvent;
static EFI_HANDLE mConnectedNetworkHandles[NETWORK_HANDLE_LIMIT];
static UINTN mConnectedNetworkHandleCount;
static uint32_t mNetworkRetryCount;


static void NetworkSerialWriteByte(uint8_t Value)
{
#if NOMADIX_ENABLE_NETWORK_DIAGNOSTICS
    uint32_t Timeout;

    for (Timeout = 0; Timeout < 1000000U; ++Timeout) {
        if ((__inbyte(COM1_BASE + 5) & COM_LSR_THRE) != 0U) {
            __outbyte(COM1_BASE, Value);
            return;
        }
    }
#else
    (void)Value;
#endif
}


static void NetworkSerialWriteString(const char *Text)
{
    while (*Text != '\0') {
        NetworkSerialWriteByte((uint8_t)*Text);
        ++Text;
    }
}


static void NetworkSerialWriteHex64(uint64_t Value)
{
    int Shift;

    for (Shift = 60; Shift >= 0; Shift -= 4) {
        uint8_t Digit;

        Digit = (uint8_t)((Value >> Shift) & 0x0FU);
        NetworkSerialWriteByte(
            (uint8_t)(Digit < 10U ? '0' + Digit : 'A' + Digit - 10U)
        );
    }
}


static uint8_t IsConnectedNetworkHandle(EFI_HANDLE Handle)
{
    UINTN Index;

    for (Index = 0; Index < mConnectedNetworkHandleCount; ++Index) {
        if (mConnectedNetworkHandles[Index] == Handle) {
            return 1U;
        }
    }
    return 0U;
}


static void RememberConnectedNetworkHandle(EFI_HANDLE Handle)
{
    if (
        mConnectedNetworkHandleCount < NETWORK_HANDLE_LIMIT &&
        IsConnectedNetworkHandle(Handle) == 0U
    ) {
        mConnectedNetworkHandles[mConnectedNetworkHandleCount] = Handle;
        ++mConnectedNetworkHandleCount;
    }
}


static void EFIAPI RetryNetworkConnections(
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
        &mNetworkInterfaceIdentifierProtocolGuid31,
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
                NetworkSerialWriteString("[NOMADIX-4GK] NII=");
                NetworkSerialWriteHex64((UINTN)Handles[Index]);
                NetworkSerialWriteString(" CONNECT=");
                NetworkSerialWriteHex64(ConnectStatus);
                NetworkSerialWriteString(" SNP=0000000000000000\r\n");
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
        NetworkSerialWriteString("[NOMADIX-4GK] COMPLETE COUNT=");
        NetworkSerialWriteHex64(mConnectedNetworkHandleCount);
        NetworkSerialWriteString(" RETRIES=");
        NetworkSerialWriteHex64(mNetworkRetryCount);
        NetworkSerialWriteString("\r\n");
    }
}


EFI_STATUS EFIAPI NOMADIX_STAGE4GK_ENTRY(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_SET_TIMER SetTimer;
    EFI_STATUS Status;

    Status = Stage4GKBaseEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    mNetworkRetryEvent = 0;
    mConnectedNetworkHandleCount = 0;
    mNetworkRetryCount = 0;
    Status = mBootServices->CreateEvent(
        EVT_TIMER | EVT_NOTIFY_SIGNAL,
        TPL_CALLBACK,
        RetryNetworkConnections,
        0,
        &mNetworkRetryEvent
    );
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    SetTimer = (EFI_SET_TIMER)mBootServices->SetTimer;
    Status = SetTimer(
        mNetworkRetryEvent,
        TIMER_PERIODIC,
        NETWORK_RETRY_INTERVAL_100NS
    );
    if (Status == EFI_SUCCESS) {
        NetworkSerialWriteString("[NOMADIX-4GK] TIMER-ARMED\r\n");
    }
    return Status;
}
