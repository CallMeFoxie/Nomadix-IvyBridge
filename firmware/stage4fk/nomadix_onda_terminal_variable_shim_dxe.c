#include <stdint.h>

static void CaptureStage4FkSystemTable(void *SystemTable);
static uint64_t InstallOndaVariableShim(void);
static void RestoreOndaVariableShim(void);

#define NOMADIX_STAGE_PREFIX "[NOMADIX-4FK]"
#define NOMADIX_CAPTURE_SYSTEM_TABLE(SystemTable) \
    CaptureStage4FkSystemTable(SystemTable)
#define NOMADIX_PREPARE_ONDA_SERIAL_VARIABLES() \
    ((EFI_STATUS)InstallOndaVariableShim())
#define NOMADIX_RESTORE_ONDA_SERIAL_VARIABLES() \
    RestoreOndaVariableShim()
#include "../stage4fi/nomadix_onda_terminal_direct_remaining_dxe.c"

#define EFI_BUFFER_TOO_SMALL EFIERR(5)

typedef EFI_STATUS (EFIAPI *EFI_GET_VARIABLE)(
    const uint16_t *VariableName,
    const EFI_GUID *VendorGuid,
    uint32_t *Attributes,
    UINTN *DataSize,
    void *Data
);

typedef struct {
    EFI_TABLE_HEADER Header;
    void *GetTime;
    void *SetTime;
    void *GetWakeupTime;
    void *SetWakeupTime;
    void *SetVirtualAddressMap;
    void *ConvertPointer;
    EFI_GET_VARIABLE GetVariable;
} EFI_RUNTIME_SERVICES_GET_VARIABLE;

typedef char AssertGetVariableOffset[
    offsetof(EFI_RUNTIME_SERVICES_GET_VARIABLE, GetVariable) == 0x48 ? 1 : -1
];

static const EFI_GUID mShimEfiSioVariableGuid = {
    0x560BF58A,
    0x1E0D,
    0x4D7E,
    {0x95, 0x3F, 0x29, 0x80, 0xA2, 0x61, 0xE0, 0x31}
};
static const uint16_t mShimSerialPortsEnabledName[] = {
    'S', 'e', 'r', 'i', 'a', 'l', 'P', 'o', 'r', 't', 's',
    'E', 'n', 'a', 'b', 'l', 'e', 'd', 'V', 'a', 'r', 0
};
static const uint16_t mShimSioSerialPortsLocationName[] = {
    'S', 'i', 'o', 'S', 'e', 'r', 'i', 'a', 'l', 'P', 'o', 'r', 't', 's',
    'L', 'o', 'c', 'a', 't', 'i', 'o', 'n', 'V', 'a', 'r', 0
};

static EFI_SYSTEM_TABLE *mStage4FkSystemTable;
static EFI_RUNTIME_SERVICES_GET_VARIABLE *mShimRuntimeServices;
static EFI_GET_VARIABLE mOriginalGetVariable;


static int GuidEquals(const EFI_GUID *Left, const EFI_GUID *Right)
{
    const uint8_t *LeftBytes;
    const uint8_t *RightBytes;
    UINTN Index;

    LeftBytes = (const uint8_t *)Left;
    RightBytes = (const uint8_t *)Right;
    for (Index = 0; Index < sizeof(EFI_GUID); ++Index) {
        if (LeftBytes[Index] != RightBytes[Index]) {
            return 0;
        }
    }
    return 1;
}


static int NameEquals(const uint16_t *Left, const uint16_t *Right)
{
    UINTN Index;

    if (Left == 0 || Right == 0) {
        return 0;
    }
    for (Index = 0; ; ++Index) {
        if (Left[Index] != Right[Index]) {
            return 0;
        }
        if (Left[Index] == 0) {
            return 1;
        }
    }
}


static EFI_STATUS ReturnShimVariable(
    const uint8_t Values[2],
    uint32_t *Attributes,
    UINTN *DataSize,
    void *Data
)
{
    uint8_t *Output;

    if (DataSize == 0) {
        return EFI_INVALID_PARAMETER;
    }
    if (*DataSize < 2U || Data == 0) {
        *DataSize = 2U;
        return EFI_BUFFER_TOO_SMALL;
    }
    if (Attributes != 0) {
        *Attributes = 0x00000006U;
    }
    Output = (uint8_t *)Data;
    Output[0] = Values[0];
    Output[1] = Values[1];
    *DataSize = 2U;
    return EFI_SUCCESS;
}


static EFI_STATUS EFIAPI ShimGetVariable(
    const uint16_t *VariableName,
    const EFI_GUID *VendorGuid,
    uint32_t *Attributes,
    UINTN *DataSize,
    void *Data
)
{
    static const uint8_t Enabled[2] = {1, 0};
    static const uint8_t Location[2] = {0, 0xFF};

    if (
        VendorGuid != 0 &&
        GuidEquals(VendorGuid, &mShimEfiSioVariableGuid)
    ) {
        if (NameEquals(VariableName, mShimSerialPortsEnabledName)) {
            SerialWriteString("[NOMADIX-4FK] GET-ENABLED\r\n");
            return ReturnShimVariable(
                Enabled,
                Attributes,
                DataSize,
                Data
            );
        }
        if (NameEquals(VariableName, mShimSioSerialPortsLocationName)) {
            SerialWriteString("[NOMADIX-4FK] GET-LOCATION\r\n");
            return ReturnShimVariable(
                Location,
                Attributes,
                DataSize,
                Data
            );
        }
    }
    return mOriginalGetVariable(
        VariableName,
        VendorGuid,
        Attributes,
        DataSize,
        Data
    );
}


static void CaptureStage4FkSystemTable(void *SystemTable)
{
    mStage4FkSystemTable = (EFI_SYSTEM_TABLE *)SystemTable;
}


static uint64_t InstallOndaVariableShim(void)
{
    if (mStage4FkSystemTable == 0) {
        return EFI_INVALID_PARAMETER;
    }
    mShimRuntimeServices =
        (EFI_RUNTIME_SERVICES_GET_VARIABLE *)
            mStage4FkSystemTable->RuntimeServices;
    if (
        mShimRuntimeServices == 0 ||
        mShimRuntimeServices->GetVariable == 0
    ) {
        return EFI_INVALID_PARAMETER;
    }
    mOriginalGetVariable = mShimRuntimeServices->GetVariable;
    mShimRuntimeServices->GetVariable = ShimGetVariable;
    SerialWriteString("[NOMADIX-4FK] VAR-SHIM-INSTALLED\r\n");
    return EFI_SUCCESS;
}


static void RestoreOndaVariableShim(void)
{
    if (mShimRuntimeServices != 0 && mOriginalGetVariable != 0) {
        mShimRuntimeServices->GetVariable = mOriginalGetVariable;
        SerialWriteString("[NOMADIX-4FK] VAR-SHIM-RESTORED\r\n");
    }
}
