#include <stdint.h>

#ifndef NOMADIX_ENABLE_DIAGNOSTIC_LOG
#define NOMADIX_ENABLE_DIAGNOSTIC_LOG 1
#endif
#ifndef NOMADIX_STAGE4GG_ENTRY
#define NOMADIX_STAGE4GG_ENTRY EfiMain
#endif
#define NOMADIX_ENABLE_PLCM_LPT 1
#define NOMADIX_PLCM_STAGE_NAME "4GG"
#define NOMADIX_STAGE4GC_ENTRY Stage4GGBaseEfiMain
#include "../stage4gc/nomadix_plcm_boot_message_dxe.c"

#define SETUP_DATA_SIZE 0x2D4U
#define RESTORE_AC_POWER_OFFSET 0x2D3U
#define GEN_PMCON_3 0xA4U
#define AFTERG3_EN 0x00000001U

static const EFI_GUID mSetupVariableGuid = {
    0xEC87D643,
    0xEBA4,
    0x4BB5,
    { 0xA1, 0xE5, 0x3F, 0x3E, 0x36, 0xB2, 0x0D, 0xA9 }
};
static const uint16_t mSetupVariableName[] = {
    'S', 'e', 't', 'u', 'p', 0
};
static EFI_EVENT mRestoreAcPowerEvent;
static uint8_t mSetupData[SETUP_DATA_SIZE];

static EFI_STATUS ApplyRestoreAcPowerPolicy(uint8_t *Setting, uint8_t *Register)
{
    EFI_STATUS Status;
    UINTN DataSize;
    uint32_t Attributes;
    uint32_t GenPmcon3;

    if (
        Setting == 0 ||
        Register == 0 ||
        mRuntimeServices == 0 ||
        mRuntimeServices->GetVariable == 0
    ) {
        return EFI_INVALID_PARAMETER;
    }
    DataSize = sizeof(mSetupData);
    Attributes = 0;
    Status = mRuntimeServices->GetVariable(
        mSetupVariableName,
        &mSetupVariableGuid,
        &Attributes,
        &DataSize,
        mSetupData
    );
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    if (
        DataSize != sizeof(mSetupData) ||
        mSetupData[RESTORE_AC_POWER_OFFSET] > 1U
    ) {
        return EFI_COMPROMISED_DATA;
    }
    *Setting = mSetupData[RESTORE_AC_POWER_OFFSET];
    GenPmcon3 = PciRead32(GEN_PMCON_3);
    if (*Setting == 1U) {
        GenPmcon3 &= ~AFTERG3_EN;
    } else {
        GenPmcon3 |= AFTERG3_EN;
    }
    PciWrite32(GEN_PMCON_3, GenPmcon3);
    GenPmcon3 = PciRead32(GEN_PMCON_3);
    *Register = (uint8_t)GenPmcon3;
    if (
        (*Setting == 1U && (GenPmcon3 & AFTERG3_EN) != 0U) ||
        (*Setting == 0U && (GenPmcon3 & AFTERG3_EN) == 0U)
    ) {
        return EFI_DEVICE_ERROR;
    }
    return EFI_SUCCESS;
}

static void EFIAPI ApplyRestoreAcPowerAtReadyToBoot(
    EFI_EVENT Event,
    void *Context
)
{
    EFI_STATUS Status;
    uint8_t Register;
    uint8_t Setting;

    (void)Event;
    (void)Context;
    Register = (uint8_t)PciRead32(GEN_PMCON_3);
    Setting = 0xFFU;
    Status = ApplyRestoreAcPowerPolicy(&Setting, &Register);
    SerialWriteString("[NOMADIX-4GG] AC-POWER=");
    SerialWriteHex64(Status);
    SerialWriteString(" SETTING=");
    SerialWriteHex64(Setting);
    SerialWriteString(" GEN_PMCON_3=");
    SerialWriteHex64(Register);
    SerialWriteString("\r\n");
}

EFI_STATUS EFIAPI NOMADIX_STAGE4GG_ENTRY(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;

    Status = Stage4GGBaseEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    mRestoreAcPowerEvent = 0;
    Status = mBootServices->CreateEvent(
        EVT_SIGNAL_READY_TO_BOOT,
        TPL_CALLBACK,
        ApplyRestoreAcPowerAtReadyToBoot,
        0,
        &mRestoreAcPowerEvent
    );
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    ApplyRestoreAcPowerAtReadyToBoot(mRestoreAcPowerEvent, 0);
    return EFI_SUCCESS;
}
