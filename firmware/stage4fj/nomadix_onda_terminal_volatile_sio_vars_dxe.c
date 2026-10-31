#include <stdint.h>

static uint64_t PrepareOndaSerialVariables(void);
static void CaptureStage4FjSystemTable(void *SystemTable);

#define NOMADIX_STAGE_PREFIX "[NOMADIX-4FJ]"
#define NOMADIX_CAPTURE_SYSTEM_TABLE(SystemTable) \
    CaptureStage4FjSystemTable(SystemTable)
#define NOMADIX_PREPARE_ONDA_SERIAL_VARIABLES() \
    ((EFI_STATUS)PrepareOndaSerialVariables())
#include "../stage4fi/nomadix_onda_terminal_direct_remaining_dxe.c"

#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002U
#define EFI_VARIABLE_RUNTIME_ACCESS 0x00000004U

typedef EFI_STATUS (EFIAPI *EFI_SET_VARIABLE)(
    const uint16_t *VariableName,
    const EFI_GUID *VendorGuid,
    uint32_t Attributes,
    UINTN DataSize,
    const void *Data
);

typedef struct {
    EFI_TABLE_HEADER Header;
    void *GetTime;
    void *SetTime;
    void *GetWakeupTime;
    void *SetWakeupTime;
    void *SetVirtualAddressMap;
    void *ConvertPointer;
    void *GetVariable;
    void *GetNextVariableName;
    EFI_SET_VARIABLE SetVariable;
} EFI_RUNTIME_SERVICES_VARIABLES;

typedef char AssertSetVariableOffset[
    offsetof(EFI_RUNTIME_SERVICES_VARIABLES, SetVariable) == 0x58 ? 1 : -1
];

static const EFI_GUID mEfiSioVariableGuid = {
    0x560BF58A,
    0x1E0D,
    0x4D7E,
    {0x95, 0x3F, 0x29, 0x80, 0xA2, 0x61, 0xE0, 0x31}
};
static const uint16_t mSerialPortsEnabledName[] = {
    'S', 'e', 'r', 'i', 'a', 'l', 'P', 'o', 'r', 't', 's',
    'E', 'n', 'a', 'b', 'l', 'e', 'd', 'V', 'a', 'r', 0
};
static const uint16_t mSioSerialPortsLocationName[] = {
    'S', 'i', 'o', 'S', 'e', 'r', 'i', 'a', 'l', 'P', 'o', 'r', 't', 's',
    'L', 'o', 'c', 'a', 't', 'i', 'o', 'n', 'V', 'a', 'r', 0
};
static EFI_SYSTEM_TABLE *mStage4FjSystemTable;


static void CaptureStage4FjSystemTable(void *SystemTable)
{
    mStage4FjSystemTable = (EFI_SYSTEM_TABLE *)SystemTable;
}


static uint64_t PrepareOndaSerialVariables(void)
{
    EFI_RUNTIME_SERVICES_VARIABLES *RuntimeServices;
    EFI_STATUS Status;
    uint8_t Enabled[2];
    uint8_t Location[2];

    RuntimeServices =
        (EFI_RUNTIME_SERVICES_VARIABLES *)
            mStage4FjSystemTable->RuntimeServices;
    Enabled[0] = 1;
    Enabled[1] = 0;
    Location[0] = 0;
    Location[1] = 0xFFU;

    Status = RuntimeServices->SetVariable(
        mSerialPortsEnabledName,
        &mEfiSioVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        sizeof(Enabled),
        Enabled
    );
    SerialWriteString("[NOMADIX-4FJ] SET-ENABLED=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        return Status;
    }

    Status = RuntimeServices->SetVariable(
        mSioSerialPortsLocationName,
        &mEfiSioVariableGuid,
        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        sizeof(Location),
        Location
    );
    SerialWriteString("[NOMADIX-4FJ] SET-LOCATION=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    return Status;
}
