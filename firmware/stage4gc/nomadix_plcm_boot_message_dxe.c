#include <stdint.h>

#ifndef NOMADIX_STAGE4GC_ENTRY
#define NOMADIX_STAGE4GC_ENTRY EfiMain
#endif

#define NOMADIX_STAGE4GB_ENTRY Stage4GCBaseEfiMain
#include "../stage4gb/nomadix_foxnodix_smbios_branding_dxe.c"

#ifndef NOMADIX_ENABLE_PLCM_LPT
#define NOMADIX_ENABLE_PLCM_LPT 0
#endif
#ifndef NOMADIX_PLCM_STAGE_NAME
#define NOMADIX_PLCM_STAGE_NAME "4GC"
#endif

#define PLCM_LPT1 0x378U
#define PLCM_LPT2 0x278U
#define PLCM_LPT3 0x3BCU
#define PLCM_ENABLE 0x02U

typedef EFI_STATUS (EFIAPI *PLCM_STALL)(UINTN Microseconds);

static uint16_t mPlcmDataPort;
static uint16_t mPlcmControlPort;
#if NOMADIX_ENABLE_PLCM_LPT
static EFI_STATUS mPlcmLptStatus;

static EFI_STATUS EnablePlcmParallelPort(void)
{
    uint16_t LpcEnable;

    if (PciRead32(0x00U) != PCH_LPC_ID) {
        return EFI_UNSUPPORTED;
    }
    LpcEnable = (uint16_t)(PciRead16(LPC_EN) | 0x0004U);
    PciWrite16(LPC_EN, LpcEnable);
    if ((PciRead16(LPC_EN) & 0x0004U) == 0U) {
        return EFI_DEVICE_ERROR;
    }

    __outbyte(SIO_INDEX, 0x87U);
    __outbyte(SIO_INDEX, 0x87U);
    if (SioRead(0x20U) != SIO_DEVICE_ID || SioRead(0x21U) != SIO_REVISION) {
        __outbyte(SIO_INDEX, 0xAAU);
        return EFI_UNSUPPORTED;
    }
    SioWrite(0x07U, 0x01U);
    SioWrite(0x30U, 0x00U);
    SioWrite(0x60U, 0x03U);
    SioWrite(0x61U, 0x78U);
    SioWrite(0x70U, 0x07U);
    SioWrite(0x30U, 0x01U);
    if (
        SioRead(0x60U) != 0x03U ||
        SioRead(0x61U) != 0x78U ||
        SioRead(0x70U) != 0x07U ||
        (SioRead(0x30U) & 0x01U) == 0U
    ) {
        __outbyte(SIO_INDEX, 0xAAU);
        return EFI_DEVICE_ERROR;
    }
    __outbyte(SIO_INDEX, 0xAAU);
    return EFI_SUCCESS;
}
#endif

static void PlcmCommand(
    uint8_t RegisterSelect,
    uint8_t Value,
    UINTN DelayMicroseconds
)
{
    PLCM_STALL Stall;
    uint8_t Control;

    Stall = (PLCM_STALL)mBootServices->Stall;
    Control = RegisterSelect == 0U ? 0x08U : 0U;
    __outbyte(mPlcmControlPort, (uint8_t)(Control | PLCM_ENABLE));
    Stall(2U);
    __outbyte(mPlcmControlPort, (uint8_t)(Control & ~PLCM_ENABLE));
    __outbyte(mPlcmDataPort, Value);
    Stall(20U);
    __outbyte(mPlcmControlPort, (uint8_t)(Control | PLCM_ENABLE));
    Stall(DelayMicroseconds + 20U);
}

static void PlcmWriteLine(uint8_t Address, const char *Text)
{
    UINTN Index;

    PlcmCommand(0U, Address, 300U);
    for (Index = 0; Index < 20U; ++Index) {
        uint8_t Character;

        Character = *Text != '\0' ? (uint8_t)*Text++ : (uint8_t)' ';
        PlcmCommand(1U, Character, 37U);
    }
}

static EFI_STATUS InitializePlcm(void)
{
    static const uint16_t Ports[] = {
        PLCM_LPT1, PLCM_LPT2, PLCM_LPT3
    };

    mPlcmDataPort = 0;
#if NOMADIX_ENABLE_PLCM_LPT
    mPlcmLptStatus = EnablePlcmParallelPort();
    if (mPlcmLptStatus != EFI_SUCCESS) {
        return mPlcmLptStatus;
    }
    mPlcmDataPort = PLCM_LPT1;
#else
    UINTN Index;

    for (Index = 0; Index < sizeof(Ports) / sizeof(Ports[0]); ++Index) {
        if (__inbyte(Ports[Index]) != 0xFFU) {
            mPlcmDataPort = Ports[Index];
            break;
        }
    }
    if (mPlcmDataPort == 0U) {
        return EFI_NOT_FOUND;
    }
#endif
    mPlcmControlPort = (uint16_t)(mPlcmDataPort + 2U);
    PlcmCommand(0U, 0x38U, 8000U);
    PlcmCommand(0U, 0x38U, 300U);
    PlcmCommand(0U, 0x38U, 300U);
    PlcmCommand(0U, 0x38U, 300U);
    PlcmCommand(0U, 0x0CU, 300U);
    PlcmCommand(0U, 0x01U, 3000U);
    PlcmCommand(0U, 0x06U, 300U);
    PlcmCommand(0U, 0x3CU, 300U);
    PlcmCommand(0U, 0x49U, 300U);
    PlcmCommand(0U, 0x38U, 300U);
    PlcmWriteLine(0x80U, "HELLO THERE");
    PlcmWriteLine(0xC0U, "FOXNODIX");
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI NOMADIX_STAGE4GC_ENTRY(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS DisplayStatus;
    EFI_STATUS Status;

    Status = Stage4GCBaseEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    DisplayStatus = InitializePlcm();
    SerialWriteString("[NOMADIX-" NOMADIX_PLCM_STAGE_NAME "] PLCM=");
    SerialWriteHex64(DisplayStatus);
#if NOMADIX_ENABLE_PLCM_LPT
    SerialWriteString(" LPT=");
    SerialWriteHex64(mPlcmLptStatus);
#endif
    SerialWriteString(" PORT=");
    SerialWriteHex64(mPlcmDataPort);
    SerialWriteString("\r\n");
    return EFI_SUCCESS;
}
