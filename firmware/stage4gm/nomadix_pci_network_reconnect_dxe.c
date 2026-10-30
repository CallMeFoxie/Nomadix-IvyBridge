#ifndef NOMADIX_STAGE4GM_ENTRY
#define NOMADIX_STAGE4GM_ENTRY EfiMain
#endif
#define NOMADIX_STAGE4GL_ENTRY Stage4GMBaseEfiMain
#include "../stage4gl/nomadix_nii_pointer_reconnect_dxe.c"
#undef NOMADIX_STAGE4GL_ENTRY


typedef struct EFI_PCI_IO_PROTOCOL EFI_PCI_IO_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_PCI_IO_PROTOCOL_CONFIG)(
    EFI_PCI_IO_PROTOCOL *This,
    uint32_t Width,
    uint32_t Offset,
    UINTN Count,
    void *Buffer
);
typedef struct {
    EFI_PCI_IO_PROTOCOL_CONFIG Read;
    EFI_PCI_IO_PROTOCOL_CONFIG Write;
} EFI_PCI_IO_PROTOCOL_ACCESS;
struct EFI_PCI_IO_PROTOCOL {
    void *PollMem;
    void *PollIo;
    EFI_PCI_IO_PROTOCOL_ACCESS Mem;
    EFI_PCI_IO_PROTOCOL_ACCESS Io;
    EFI_PCI_IO_PROTOCOL_ACCESS Pci;
};

#define EFI_PCI_IO_WIDTH_UINT32 2U
#define INTEL_VENDOR_ID 0x8086U
#define INTEL_82583V_DEVICE_ID 0x150CU

static const EFI_GUID mPciIoProtocolGuid = {
    0x4CF5B200,
    0x68B8,
    0x4CA5,
    { 0x9E, 0xEC, 0xB2, 0x3E, 0x3F, 0x50, 0x02, 0x9A }
};
static EFI_HANDLE mConnectedPciHandles[NETWORK_HANDLE_LIMIT];
static UINTN mConnectedPciHandleCount;


static uint8_t IsConnectedPciHandle(EFI_HANDLE Handle)
{
    UINTN Index;

    for (Index = 0; Index < mConnectedPciHandleCount; ++Index) {
        if (mConnectedPciHandles[Index] == Handle) {
            return 1U;
        }
    }
    return 0U;
}


static void ConnectIntelPciParents(void)
{
    EFI_CONNECT_CONTROLLER ConnectController;
    EFI_HANDLE *Handles;
    EFI_STATUS LocateStatus;
    UINTN Count;
    UINTN Index;

    Handles = 0;
    Count = 0;
    LocateStatus = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mPciIoProtocolGuid,
        0,
        &Count,
        &Handles
    );
    if (LocateStatus != EFI_SUCCESS) {
        return;
    }
    ConnectController =
        (EFI_CONNECT_CONTROLLER)mBootServices->ConnectController;
    for (Index = 0; Index < Count; ++Index) {
        EFI_PCI_IO_PROTOCOL *PciIo;
        EFI_STATUS ConnectStatus;
        EFI_STATUS Status;
        uint32_t VendorDevice;

        if (IsConnectedPciHandle(Handles[Index]) != 0U) {
            continue;
        }
        PciIo = 0;
        Status = mBootServices->HandleProtocol(
            Handles[Index],
            &mPciIoProtocolGuid,
            (void **)&PciIo
        );
        if (Status != EFI_SUCCESS || PciIo == 0 || PciIo->Pci.Read == 0) {
            continue;
        }
        VendorDevice = 0xFFFFFFFFU;
        Status = PciIo->Pci.Read(
            PciIo,
            EFI_PCI_IO_WIDTH_UINT32,
            0,
            1,
            &VendorDevice
        );
        if (
            Status != EFI_SUCCESS ||
            (VendorDevice & 0xFFFFU) != INTEL_VENDOR_ID ||
            (VendorDevice >> 16) != INTEL_82583V_DEVICE_ID
        ) {
            continue;
        }
        ConnectStatus = ConnectController(Handles[Index], 0, 0, 1U);
        if (
            ConnectStatus == EFI_SUCCESS &&
            mConnectedPciHandleCount < NETWORK_HANDLE_LIMIT
        ) {
            mConnectedPciHandles[mConnectedPciHandleCount] = Handles[Index];
            ++mConnectedPciHandleCount;
            NetworkSerialWriteString("[NOMADIX-4GM] PCI=");
            NetworkSerialWriteHex64((UINTN)Handles[Index]);
            NetworkSerialWriteString(" CONNECT=0000000000000000\r\n");
        }
    }
    mBootServices->FreePool(Handles);
}


static void ConnectNiiChildren(void)
{
    EFI_CONNECT_CONTROLLER ConnectController;
    EFI_HANDLE *Handles;
    EFI_STATUS LocateStatus;
    UINTN Count;
    UINTN Index;

    Handles = 0;
    Count = 0;
    LocateStatus = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mNetworkInterfaceIdentifierProtocolGuid31,
        0,
        &Count,
        &Handles
    );
    if (LocateStatus != EFI_SUCCESS) {
        return;
    }
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
            NetworkSerialWriteString("[NOMADIX-4GM] SNP=");
            NetworkSerialWriteHex64((UINTN)Handles[Index]);
            NetworkSerialWriteString(" CONNECT=");
            NetworkSerialWriteHex64(ConnectStatus);
            NetworkSerialWriteString("\r\n");
        }
    }
    mBootServices->FreePool(Handles);
}


static void EFIAPI ConnectIntelNetworkStack(
    EFI_EVENT Event,
    void *Context
)
{
    EFI_SET_TIMER SetTimer;

    (void)Context;
    ++mNetworkRetryCount;
    ConnectIntelPciParents();
    ConnectNiiChildren();
    if (
        mConnectedNetworkHandleCount >= 6U ||
        mNetworkRetryCount >= NETWORK_RETRY_LIMIT
    ) {
        SetTimer = (EFI_SET_TIMER)mBootServices->SetTimer;
        SetTimer(Event, TIMER_CANCEL, 0);
        NetworkSerialWriteString("[NOMADIX-4GM] COMPLETE PCI=");
        NetworkSerialWriteHex64(mConnectedPciHandleCount);
        NetworkSerialWriteString(" SNP=");
        NetworkSerialWriteHex64(mConnectedNetworkHandleCount);
        NetworkSerialWriteString(" RETRIES=");
        NetworkSerialWriteHex64(mNetworkRetryCount);
        NetworkSerialWriteString("\r\n");
    }
}


EFI_STATUS EFIAPI NOMADIX_STAGE4GM_ENTRY(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_CLOSE_EVENT CloseEvent;
    EFI_SET_TIMER SetTimer;
    EFI_STATUS Status;

    Status = Stage4GMBaseEfiMain(ImageHandle, SystemTable);
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
    mConnectedPciHandleCount = 0;
    mNetworkRetryCount = 0;
    Status = mBootServices->CreateEvent(
        EVT_TIMER | EVT_NOTIFY_SIGNAL,
        TPL_CALLBACK,
        ConnectIntelNetworkStack,
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
        NetworkSerialWriteString("[NOMADIX-4GM] TIMER-ARMED\r\n");
    }
    return Status;
}
