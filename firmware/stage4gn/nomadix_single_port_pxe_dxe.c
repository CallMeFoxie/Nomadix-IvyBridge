#ifndef NOMADIX_STAGE4GN_ENTRY
#define NOMADIX_STAGE4GN_ENTRY EfiMain
#endif
#define NOMADIX_STAGE4GM_ENTRY Stage4GNBaseEfiMain
#include "../stage4gm/nomadix_pci_network_reconnect_dxe.c"
#undef NOMADIX_STAGE4GM_ENTRY


typedef struct EFI_DRIVER_BINDING_PROTOCOL EFI_DRIVER_BINDING_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_DRIVER_BINDING_SUPPORTED)(
    EFI_DRIVER_BINDING_PROTOCOL *This,
    EFI_HANDLE ControllerHandle,
    void *RemainingDevicePath
);
typedef EFI_STATUS (EFIAPI *EFI_PCI_IO_PROTOCOL_GET_LOCATION)(
    EFI_PCI_IO_PROTOCOL *This,
    UINTN *SegmentNumber,
    UINTN *BusNumber,
    UINTN *DeviceNumber,
    UINTN *FunctionNumber
);
typedef EFI_STATUS (EFIAPI *EFI_HII_DATABASE_REMOVE_PACKAGE_LIST)(
    const void *This,
    void *Handle
);
typedef EFI_STATUS (EFIAPI *EFI_HII_DATABASE_LIST_PACKS)(
    const void *This,
    uint8_t PackageType,
    const EFI_GUID *PackageGuid,
    UINTN *HandleBufferLength,
    void **Handle
);
typedef EFI_STATUS (EFIAPI *EFI_HII_DATABASE_EXPORT_PACKS)(
    const void *This,
    void *Handle,
    UINTN *BufferSize,
    void *Buffer
);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(
    const EFI_GUID *Protocol,
    void *Registration,
    void **Interface
);
typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL)(
    uint32_t PoolType,
    UINTN Size,
    void **Buffer
);

struct EFI_DRIVER_BINDING_PROTOCOL {
    EFI_DRIVER_BINDING_SUPPORTED Supported;
    void *Start;
    void *Stop;
    uint32_t Version;
    EFI_HANDLE ImageHandle;
    EFI_HANDLE DriverBindingHandle;
};
typedef struct {
    void *NewPackageList;
    EFI_HII_DATABASE_REMOVE_PACKAGE_LIST RemovePackageList;
    void *UpdatePackageList;
    EFI_HII_DATABASE_LIST_PACKS ListPackageLists;
    EFI_HII_DATABASE_EXPORT_PACKS ExportPackageLists;
} EFI_HII_DATABASE_PROTOCOL_MIN;

#define INTEL_UNDI_DRIVER_VERSION 0x09092500U
#define EFI_HII_PACKAGE_TYPE_ALL 0U
#define EFI_BOOT_SERVICES_DATA 4U
#define EFI_BUFFER_TOO_SMALL 0x8000000000000005ULL
#define EFI_BOOT_SERVICES_LOCATE_PROTOCOL_OFFSET 320U

static const EFI_GUID mDriverBindingProtocolGuidSinglePort = {
    0x18A031AB,
    0xB443,
    0x4D1A,
    { 0xA5, 0xC0, 0x0C, 0x09, 0x26, 0x1E, 0x9F, 0x71 }
};
static const EFI_GUID mHiiDatabaseProtocolGuidSinglePort = {
    0xEF9FC172,
    0xA1B2,
    0x4693,
    { 0xB3, 0x27, 0x6D, 0x32, 0xFC, 0x41, 0x60, 0x42 }
};
static const EFI_GUID mIntelHiiFormGuid = {
    0x77F2EA2F,
    0x4312,
    0x4569,
    { 0x85, 0xC4, 0x58, 0x3A, 0xCD, 0x8D, 0xB7, 0xE2 }
};
static EFI_DRIVER_BINDING_SUPPORTED mOriginalIntelSupported;
static EFI_DRIVER_BINDING_PROTOCOL *mIntelBinding;
static EFI_HANDLE mPrimaryNicHandle;
static uint8_t mIntelHiiRemoved;


static uint8_t GuidEqual(const EFI_GUID *Left, const EFI_GUID *Right)
{
    const uint8_t *LeftBytes;
    const uint8_t *RightBytes;
    UINTN Index;

    LeftBytes = (const uint8_t *)Left;
    RightBytes = (const uint8_t *)Right;
    for (Index = 0; Index < sizeof(EFI_GUID); ++Index) {
        if (LeftBytes[Index] != RightBytes[Index]) {
            return 0U;
        }
    }
    return 1U;
}


static EFI_STATUS GetPciLocation(
    EFI_HANDLE Handle,
    UINTN *Segment,
    UINTN *Bus,
    UINTN *Device,
    UINTN *Function
)
{
    EFI_PCI_IO_PROTOCOL *PciIo;
    EFI_PCI_IO_PROTOCOL_GET_LOCATION GetLocation;
    EFI_STATUS Status;
    void **ProtocolEntries;

    PciIo = 0;
    Status = mBootServices->HandleProtocol(
        Handle,
        &mPciIoProtocolGuid,
        (void **)&PciIo
    );
    if (Status != EFI_SUCCESS || PciIo == 0) {
        return Status;
    }
    ProtocolEntries = (void **)PciIo;
    GetLocation = (EFI_PCI_IO_PROTOCOL_GET_LOCATION)ProtocolEntries[14];
    if (GetLocation == 0) {
        return EFI_UNSUPPORTED;
    }
    return GetLocation(PciIo, Segment, Bus, Device, Function);
}


static uint8_t IsPrimaryNic(EFI_HANDLE ControllerHandle)
{
    UINTN Bus;
    UINTN Device;
    UINTN Function;
    UINTN Segment;

    Segment = 0;
    Bus = 0;
    Device = 0;
    Function = 0;
    if (
        GetPciLocation(
            ControllerHandle,
            &Segment,
            &Bus,
            &Device,
            &Function
        ) != EFI_SUCCESS
    ) {
        return 0U;
    }
    return (uint8_t)(
        Segment == 0U && Bus == 1U && Device == 0U && Function == 0U
    );
}


static EFI_STATUS EFIAPI SinglePortIntelSupported(
    EFI_DRIVER_BINDING_PROTOCOL *This,
    EFI_HANDLE ControllerHandle,
    void *RemainingDevicePath
)
{
    EFI_PCI_IO_PROTOCOL *PciIo;
    EFI_STATUS Status;
    uint32_t VendorDevice;

    PciIo = 0;
    Status = mBootServices->HandleProtocol(
        ControllerHandle,
        &mPciIoProtocolGuid,
        (void **)&PciIo
    );
    if (Status == EFI_SUCCESS && PciIo != 0 && PciIo->Pci.Read != 0) {
        VendorDevice = 0xFFFFFFFFU;
        Status = PciIo->Pci.Read(
            PciIo,
            EFI_PCI_IO_WIDTH_UINT32,
            0,
            1,
            &VendorDevice
        );
        if (
            Status == EFI_SUCCESS &&
            (VendorDevice & 0xFFFFU) == INTEL_VENDOR_ID &&
            (VendorDevice >> 16) == INTEL_82583V_DEVICE_ID &&
            IsPrimaryNic(ControllerHandle) == 0U
        ) {
            return EFI_UNSUPPORTED;
        }
    }
    return mOriginalIntelSupported(
        This,
        ControllerHandle,
        RemainingDevicePath
    );
}


static EFI_STATUS InstallSinglePortFilter(void)
{
    EFI_DRIVER_BINDING_PROTOCOL *Binding;
    EFI_HANDLE *Handles;
    EFI_STATUS Status;
    UINTN Count;
    UINTN Index;

    Handles = 0;
    Count = 0;
    Status = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mDriverBindingProtocolGuidSinglePort,
        0,
        &Count,
        &Handles
    );
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = EFI_NOT_FOUND;
    for (Index = 0; Index < Count; ++Index) {
        Binding = 0;
        if (
            mBootServices->HandleProtocol(
                Handles[Index],
                &mDriverBindingProtocolGuidSinglePort,
                (void **)&Binding
            ) != EFI_SUCCESS ||
            Binding == 0 ||
            Binding->Version != INTEL_UNDI_DRIVER_VERSION
        ) {
            continue;
        }
        mIntelBinding = Binding;
        mOriginalIntelSupported = Binding->Supported;
        Binding->Supported = SinglePortIntelSupported;
        Status = EFI_SUCCESS;
        break;
    }
    mBootServices->FreePool(Handles);
    return Status;
}


static void RemoveIntelHiiPackages(void)
{
    EFI_ALLOCATE_POOL AllocatePool;
    EFI_HII_DATABASE_PROTOCOL_MIN *Database;
    EFI_LOCATE_PROTOCOL LocateProtocol;
    EFI_STATUS Status;
    UINTN HandleBufferLength;
    UINTN Index;
    void **Handles;

    if (mIntelHiiRemoved != 0U) {
        return;
    }
    LocateProtocol = *(EFI_LOCATE_PROTOCOL *)(
        (uint8_t *)mBootServices + EFI_BOOT_SERVICES_LOCATE_PROTOCOL_OFFSET
    );
    Database = 0;
    Status = LocateProtocol(
        &mHiiDatabaseProtocolGuidSinglePort,
        0,
        (void **)&Database
    );
    if (Status != EFI_SUCCESS || Database == 0) {
        return;
    }
    AllocatePool = (EFI_ALLOCATE_POOL)mBootServices->AllocatePool;
    Handles = 0;
    HandleBufferLength = 0;
    Status = Database->ListPackageLists(
        Database,
        EFI_HII_PACKAGE_TYPE_ALL,
        0,
        &HandleBufferLength,
        0
    );
    if (
        Status != EFI_BUFFER_TOO_SMALL ||
        HandleBufferLength == 0 ||
        AllocatePool(
            EFI_BOOT_SERVICES_DATA,
            HandleBufferLength,
            (void **)&Handles
        ) != EFI_SUCCESS
    ) {
        return;
    }
    Status = Database->ListPackageLists(
        Database,
        EFI_HII_PACKAGE_TYPE_ALL,
        0,
        &HandleBufferLength,
        Handles
    );
    if (Status != EFI_SUCCESS) {
        mBootServices->FreePool(Handles);
        return;
    }
    for (Index = 0; Index < HandleBufferLength / sizeof(Handles[0]); ++Index) {
        EFI_GUID *PackageListGuid;
        UINTN PackageListSize;
        void *PackageList;

        PackageList = 0;
        PackageListSize = 0;
        Status = Database->ExportPackageLists(
            Database,
            Handles[Index],
            &PackageListSize,
            0
        );
        if (
            Status != EFI_BUFFER_TOO_SMALL ||
            PackageListSize < sizeof(EFI_GUID) ||
            AllocatePool(
                EFI_BOOT_SERVICES_DATA,
                PackageListSize,
                &PackageList
            ) != EFI_SUCCESS
        ) {
            continue;
        }
        Status = Database->ExportPackageLists(
            Database,
            Handles[Index],
            &PackageListSize,
            PackageList
        );
        PackageListGuid = (EFI_GUID *)PackageList;
        if (
            Status == EFI_SUCCESS &&
            GuidEqual(PackageListGuid, &mIntelHiiFormGuid) != 0U
        ) {
            Database->RemovePackageList(Database, Handles[Index]);
            mIntelHiiRemoved = 1U;
        }
        mBootServices->FreePool(PackageList);
    }
    mBootServices->FreePool(Handles);
    if (mIntelHiiRemoved != 0U) {
        NetworkSerialWriteString("[NOMADIX-4GN] INTEL-HII-REMOVED\r\n");
    }
}


static void EFIAPI ConnectPrimaryNetworkStack(
    EFI_EVENT Event,
    void *Context
)
{
    EFI_CONNECT_CONTROLLER ConnectController;
    EFI_HANDLE *Handles;
    EFI_SET_TIMER SetTimer;
    EFI_STATUS Status;
    UINTN Count;
    UINTN Index;

    (void)Context;
    ++mNetworkRetryCount;
    Handles = 0;
    Count = 0;
    Status = mBootServices->LocateHandleBuffer(
        EFI_BY_PROTOCOL,
        &mPciIoProtocolGuid,
        0,
        &Count,
        &Handles
    );
    if (Status == EFI_SUCCESS) {
        ConnectController =
            (EFI_CONNECT_CONTROLLER)mBootServices->ConnectController;
        for (Index = 0; Index < Count; ++Index) {
            if (IsPrimaryNic(Handles[Index]) != 0U) {
                mPrimaryNicHandle = Handles[Index];
                ConnectController(mPrimaryNicHandle, 0, 0, 1U);
                break;
            }
        }
        mBootServices->FreePool(Handles);
    }
    ConnectNiiChildren();
    RemoveIntelHiiPackages();
    if (
        mConnectedNetworkHandleCount >= 1U ||
        mNetworkRetryCount >= NETWORK_RETRY_LIMIT
    ) {
        SetTimer = (EFI_SET_TIMER)mBootServices->SetTimer;
        SetTimer(Event, TIMER_CANCEL, 0);
        NetworkSerialWriteString("[NOMADIX-4GN] COMPLETE SNP=");
        NetworkSerialWriteHex64(mConnectedNetworkHandleCount);
        NetworkSerialWriteString(" RETRIES=");
        NetworkSerialWriteHex64(mNetworkRetryCount);
        NetworkSerialWriteString("\r\n");
    }
}


EFI_STATUS EFIAPI NOMADIX_STAGE4GN_ENTRY(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_CLOSE_EVENT CloseEvent;
    EFI_SET_TIMER SetTimer;
    EFI_STATUS FilterStatus;
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
    FilterStatus = InstallSinglePortFilter();
    NetworkSerialWriteString("[NOMADIX-4GN] FILTER=");
    NetworkSerialWriteHex64(FilterStatus);
    NetworkSerialWriteString("\r\n");
    if (FilterStatus != EFI_SUCCESS) {
        return FilterStatus;
    }
    mNetworkRetryEvent = 0;
    mConnectedNetworkHandleCount = 0;
    mNetworkRetryCount = 0;
    mPrimaryNicHandle = 0;
    mIntelHiiRemoved = 0;
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
    Status = SetTimer(
        mNetworkRetryEvent,
        TIMER_PERIODIC,
        NETWORK_RETRY_INTERVAL_100NS
    );
    if (Status == EFI_SUCCESS) {
        NetworkSerialWriteString("[NOMADIX-4GN] TIMER-ARMED\r\n");
    }
    return Status;
}
