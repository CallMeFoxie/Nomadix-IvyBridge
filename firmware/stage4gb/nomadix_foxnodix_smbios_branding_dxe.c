#include <stdint.h>

#ifndef NOMADIX_STAGE4GB_ENTRY
#define NOMADIX_STAGE4GB_ENTRY EfiMain
#endif

#define NOMADIX_STAGE4FY_ENTRY Stage4GBBaseEfiMain
#define NOMADIX_STAGE4FY_MARKER_GROUP 'G'
#define NOMADIX_STAGE4FY_MARKER_STAGE 'B'
#define NOMADIX_ENABLE_FAN_PROFILES 1
#define NOMADIX_PERSIST_FAN_PROFILE 1
#include "../stage4fy/nomadix_w83627_hii_formset_dxe.c"

#define SMBIOS_HANDLE_PI_RESERVED 0xFFFEU

typedef uint16_t EFI_SMBIOS_HANDLE;
typedef uint8_t EFI_SMBIOS_TYPE;

typedef struct {
    uint8_t Type;
    uint8_t Length;
    EFI_SMBIOS_HANDLE Handle;
} EFI_SMBIOS_TABLE_HEADER;

typedef struct EFI_SMBIOS_PROTOCOL EFI_SMBIOS_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_SMBIOS_UPDATE_STRING)(
    const EFI_SMBIOS_PROTOCOL *This,
    EFI_SMBIOS_HANDLE *SmbiosHandle,
    UINTN *StringNumber,
    char *String
);
typedef EFI_STATUS (EFIAPI *EFI_SMBIOS_GET_NEXT)(
    const EFI_SMBIOS_PROTOCOL *This,
    EFI_SMBIOS_HANDLE *SmbiosHandle,
    EFI_SMBIOS_TYPE *Type,
    EFI_SMBIOS_TABLE_HEADER **Record,
    EFI_HANDLE *ProducerHandle
);

struct EFI_SMBIOS_PROTOCOL {
    void *Add;
    EFI_SMBIOS_UPDATE_STRING UpdateString;
    void *Remove;
    EFI_SMBIOS_GET_NEXT GetNext;
    uint8_t MajorVersion;
    uint8_t MinorVersion;
};

static const EFI_GUID mSmbiosProtocolGuid = {
    0x03583FF6,
    0xCB36,
    0x4940,
    { 0x94, 0x7E, 0xB9, 0xB3, 0x9F, 0x4A, 0xFA, 0xF7 }
};

static EFI_EVENT mSmbiosBrandEvent;
static EFI_SMBIOS_PROTOCOL *mSmbios;
static uint8_t mSmbiosBranded;
static UINTN mSmbiosUpdated;
static UINTN mSmbiosSkipped;

static EFI_STATUS UpdateSmbiosString(
    EFI_SMBIOS_HANDLE Handle,
    uint8_t StringNumber,
    const char *Value
)
{
    EFI_STATUS Status;
    UINTN Number;

    if (StringNumber == 0U) {
        ++mSmbiosSkipped;
        return EFI_NOT_FOUND;
    }
    Number = StringNumber;
    Status = mSmbios->UpdateString(
        mSmbios,
        &Handle,
        &Number,
        (char *)Value
    );
    if (Status == EFI_SUCCESS) {
        ++mSmbiosUpdated;
    }
    return Status;
}

static EFI_STATUS BrandSmbiosType(
    EFI_SMBIOS_TYPE Type,
    uint8_t ManufacturerOffset,
    uint8_t ProductOffset,
    uint8_t VersionOffset,
    const char *Manufacturer,
    const char *Product,
    const char *Version
)
{
    EFI_SMBIOS_HANDLE Handle;
    EFI_SMBIOS_HANDLE RecordHandle;
    EFI_SMBIOS_TABLE_HEADER *Record;
    EFI_STATUS FirstError;
    EFI_STATUS Status;
    uint8_t ManufacturerIndex;
    uint8_t ProductIndex;
    uint8_t VersionIndex;

    Handle = SMBIOS_HANDLE_PI_RESERVED;
    Record = 0;
    Status = mSmbios->GetNext(mSmbios, &Handle, &Type, &Record, 0);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    if (
        Record == 0 ||
        Record->Length <= ManufacturerOffset ||
        Record->Length <= VersionOffset ||
        (Product != 0 && Record->Length <= ProductOffset)
    ) {
        return EFI_DEVICE_ERROR;
    }
    ManufacturerIndex = ((uint8_t *)Record)[ManufacturerOffset];
    ProductIndex =
        Product != 0 ? ((uint8_t *)Record)[ProductOffset] : 0;
    VersionIndex = ((uint8_t *)Record)[VersionOffset];
    RecordHandle = Record->Handle;
    FirstError = UpdateSmbiosString(
        RecordHandle,
        ManufacturerIndex,
        Manufacturer
    );
    if (Product != 0) {
        Status = UpdateSmbiosString(RecordHandle, ProductIndex, Product);
        if (FirstError == EFI_SUCCESS && Status != EFI_SUCCESS) {
            FirstError = Status;
        }
    }
    Status = UpdateSmbiosString(RecordHandle, VersionIndex, Version);
    if (
        FirstError == EFI_SUCCESS &&
        Status != EFI_SUCCESS &&
        VersionIndex != 0U
    ) {
        FirstError = Status;
    }
    return FirstError;
}

static void EFIAPI BrandSmbios(EFI_EVENT Event, void *Context)
{
    EFI_LOCATE_PROTOCOL LocateProtocol;
    EFI_STATUS FirstError;
    EFI_STATUS Status;

    (void)Event;
    (void)Context;
    if (mSmbiosBranded != 0U) {
        return;
    }
    LocateProtocol = GetLocateProtocol();
    Status = LocateProtocol(&mSmbiosProtocolGuid, 0, (void **)&mSmbios);
    if (Status != EFI_SUCCESS) {
        return;
    }
    mSmbiosUpdated = 0;
    mSmbiosSkipped = 0;
    FirstError = BrandSmbiosType(
        0U,
        4U,
        0U,
        5U,
        "FOXNODIX",
        0,
        "Nomadix Ivy Edition"
    );
    Status = BrandSmbiosType(
        1U,
        4U,
        5U,
        6U,
        "FOXNODIX",
        "FW-7582",
        "Nomadix Ivy Edition"
    );
    if (FirstError == EFI_SUCCESS && Status != EFI_SUCCESS) {
        FirstError = Status;
    }
    Status = BrandSmbiosType(
        2U,
        4U,
        5U,
        6U,
        "FOXNODIX",
        "FW-7582 Mainboard",
        "Nomadix Ivy Edition"
    );
    if (FirstError == EFI_SUCCESS && Status != EFI_SUCCESS) {
        FirstError = Status;
    }
    Status = BrandSmbiosType(
        3U,
        4U,
        0U,
        6U,
        "FOXNODIX",
        0,
        "Nomadix Ivy Edition"
    );
    if (FirstError == EFI_SUCCESS && Status != EFI_SUCCESS) {
        FirstError = Status;
    }
    SerialWriteString("[NOMADIX-4GB] SMBIOS=");
    SerialWriteHex64(FirstError);
    SerialWriteString(" UPDATED=");
    SerialWriteHex64(mSmbiosUpdated);
    SerialWriteString(" SKIPPED=");
    SerialWriteHex64(mSmbiosSkipped);
    SerialWriteString("\r\n");
    mSmbiosBranded = 1U;
}

EFI_STATUS EFIAPI NOMADIX_STAGE4GB_ENTRY(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;

    Status = Stage4GBBaseEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    mSmbiosBrandEvent = 0;
    Status = mBootServices->CreateEvent(
        EVT_TIMER | EVT_NOTIFY_SIGNAL,
        TPL_CALLBACK,
        BrandSmbios,
        0,
        &mSmbiosBrandEvent
    );
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = GetSetTimer()(mSmbiosBrandEvent, TIMER_PERIODIC, 100000U);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    BrandSmbios(mSmbiosBrandEvent, 0);
    return EFI_SUCCESS;
}
