#include <intrin.h>
#include <stdint.h>


typedef uint64_t EFI_STATUS;
typedef void *EFI_HANDLE;
typedef struct EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;

#define EFIAPI
#define EFI_SUCCESS 0
#define EFIERR(code) (0x8000000000000000ULL | (code))
#define EFI_UNSUPPORTED EFIERR(3)
#define EFI_DEVICE_ERROR EFIERR(7)

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC
#define PCH_LPC_BUS 0
#define PCH_LPC_DEVICE 31
#define PCH_LPC_FUNCTION 0
#define PCH_LPC_ID 0x1C5C8086U
#define LPC_IO_DEC 0x80
#define LPC_EN 0x82
#define GEN2_DEC 0x88
#define HWM_DECODE 0x000C0291U

#define SIO_INDEX 0x2E
#define SIO_DATA 0x2F
#define SIO_DEVICE_ID 0xB0
#define SIO_REVISION 0x73
#define SIO_HWM_LDN 0x0B

#define HWM_INDEX 0x295
#define HWM_DATA 0x296


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
);

static void *volatile mImageRelocation = (void *)&EfiMain;


static uint32_t PciAddress(
    uint8_t Bus,
    uint8_t Device,
    uint8_t Function,
    uint8_t Register
)
{
    return 0x80000000U |
           ((uint32_t)Bus << 16) |
           ((uint32_t)Device << 11) |
           ((uint32_t)Function << 8) |
           (Register & 0xFCU);
}


static uint32_t PciRead32(uint8_t Register)
{
    __outdword(
        PCI_CONFIG_ADDRESS,
        PciAddress(PCH_LPC_BUS, PCH_LPC_DEVICE, PCH_LPC_FUNCTION, Register)
    );
    return __indword(PCI_CONFIG_DATA);
}


static void PciWrite32(uint8_t Register, uint32_t Value)
{
    __outdword(
        PCI_CONFIG_ADDRESS,
        PciAddress(PCH_LPC_BUS, PCH_LPC_DEVICE, PCH_LPC_FUNCTION, Register)
    );
    __outdword(PCI_CONFIG_DATA, Value);
}


static uint16_t PciRead16(uint8_t Register)
{
    uint32_t Value = PciRead32(Register);
    return (uint16_t)(Value >> ((Register & 2U) * 8U));
}


static void PciWrite16(uint8_t Register, uint16_t Value)
{
    uint32_t Shift = (Register & 2U) * 8U;
    uint32_t Current = PciRead32(Register);
    uint32_t Mask = 0xFFFFU << Shift;
    PciWrite32(Register, (Current & ~Mask) | ((uint32_t)Value << Shift));
}


static void SioWrite(uint8_t Register, uint8_t Value)
{
    __outbyte(SIO_INDEX, Register);
    __outbyte(SIO_DATA, Value);
}


static uint8_t SioRead(uint8_t Register)
{
    __outbyte(SIO_INDEX, Register);
    return __inbyte(SIO_DATA);
}


static void HwmWrite(uint8_t Register, uint8_t Value)
{
    __outbyte(HWM_INDEX, Register);
    __outbyte(HWM_DATA, Value);
}


static uint8_t HwmRead(uint8_t Register)
{
    __outbyte(HWM_INDEX, Register);
    return __inbyte(HWM_DATA);
}


static EFI_STATUS InitializePchDecode(void)
{
    uint32_t GenericDecode;

    if (PciRead32(0x00) != PCH_LPC_ID) {
        return EFI_UNSUPPORTED;
    }

    PciWrite16(LPC_IO_DEC, (uint16_t)(PciRead16(LPC_IO_DEC) | 0x0010U));
    PciWrite16(LPC_EN, (uint16_t)(PciRead16(LPC_EN) | 0x0003U));

    GenericDecode = PciRead32(GEN2_DEC);
    if ((GenericDecode & 1U) != 0U && GenericDecode != HWM_DECODE) {
        return EFI_DEVICE_ERROR;
    }
    PciWrite32(GEN2_DEC, HWM_DECODE);
    if ((PciRead16(LPC_IO_DEC) & 0x0010U) == 0U ||
        (PciRead16(LPC_EN) & 0x0003U) != 0x0003U ||
        PciRead32(GEN2_DEC) != HWM_DECODE) {
        return EFI_DEVICE_ERROR;
    }
    return EFI_SUCCESS;
}


static EFI_STATUS InitializeSuperIo(void)
{
    __outbyte(SIO_INDEX, 0x87);
    __outbyte(SIO_INDEX, 0x87);

    if (SioRead(0x20) != SIO_DEVICE_ID || SioRead(0x21) != SIO_REVISION) {
        __outbyte(SIO_INDEX, 0xAA);
        return EFI_UNSUPPORTED;
    }

    SioWrite(0x07, SIO_HWM_LDN);
    SioWrite(0x60, 0x02);
    SioWrite(0x61, 0x90);
    SioWrite(0xF0, 0x80);
    SioWrite(0xF2, 0x80);
    SioWrite(0x30, 0x01);
    if (SioRead(0x60) != 0x02 ||
        SioRead(0x61) != 0x90 ||
        SioRead(0xF0) != 0x80 ||
        SioRead(0xF2) != 0x80 ||
        (SioRead(0x30) & 1U) == 0U) {
        __outbyte(SIO_INDEX, 0xAA);
        return EFI_DEVICE_ERROR;
    }
    __outbyte(SIO_INDEX, 0xAA);

    HwmWrite(0x4E, 0x00);
    HwmWrite(0x40, 0x03);
    HwmWrite(0x04, 0x00);
    HwmWrite(0x12, 0x78);
    HwmWrite(0x62, 0x00);
    HwmWrite(0x01, 0xFF);
    HwmWrite(0x03, 0xFF);
    HwmWrite(0x11, 0xFF);
    HwmWrite(0x61, 0xFF);
    if ((HwmRead(0x40) & 0x03U) != 0x03U ||
        HwmRead(0x01) != 0xFF ||
        HwmRead(0x03) != 0xFF ||
        HwmRead(0x11) != 0xFF ||
        HwmRead(0x61) != 0xFF) {
        return EFI_DEVICE_ERROR;
    }
    return EFI_SUCCESS;
}


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;

    (void)ImageHandle;
    (void)SystemTable;

    if (mImageRelocation == 0) {
        return EFI_DEVICE_ERROR;
    }

    Status = InitializePchDecode();
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    return InitializeSuperIo();
}
