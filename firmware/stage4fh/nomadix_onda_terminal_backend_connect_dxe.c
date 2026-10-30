#include <stdint.h>

static void PrepareAmiPrivateBackend(uint8_t *Container);

#ifndef NOMADIX_STAGE4FF_ENTRY
#define NOMADIX_STAGE4FF_ENTRY EfiMain
#endif
#ifndef NOMADIX_STAGE_PREFIX
#define NOMADIX_STAGE_PREFIX "[NOMADIX-4FH]"
#endif
#define NOMADIX_DEFER_ONDA_TERMINAL_ARM
#define NOMADIX_PREPARE_AMI_PRIVATE(Container) \
    PrepareAmiPrivateBackend(Container)
#include "../stage4ff/nomadix_onda_terminal_standard_connect_dxe.c"

typedef EFI_STATUS (EFIAPI *AMI_PCI_IO_ACCESS)(
    void *This,
    uint32_t Width,
    uint8_t BarIndex,
    uint64_t Offset,
    UINTN Count,
    void *Buffer
);

typedef struct {
    void *PollMem;
    void *PollIo;
    void *MemRead;
    void *MemWrite;
    AMI_PCI_IO_ACCESS IoRead;
    AMI_PCI_IO_ACCESS IoWrite;
} AMI_PCI_IO_BACKEND;

typedef char AssertAmiIoReadOffset[
    offsetof(AMI_PCI_IO_BACKEND, IoRead) == 0x20 ? 1 : -1
];
typedef char AssertAmiIoWriteOffset[
    offsetof(AMI_PCI_IO_BACKEND, IoWrite) == 0x28 ? 1 : -1
];

static EFI_STATUS EFIAPI AmiBackendIoRead(
    void *This,
    uint32_t Width,
    uint8_t BarIndex,
    uint64_t Offset,
    UINTN Count,
    void *Buffer
);
static EFI_STATUS EFIAPI AmiBackendIoWrite(
    void *This,
    uint32_t Width,
    uint8_t BarIndex,
    uint64_t Offset,
    UINTN Count,
    void *Buffer
);

static AMI_PCI_IO_BACKEND mAmiPciIoBackend = {
    0,
    0,
    0,
    0,
    AmiBackendIoRead,
    AmiBackendIoWrite
};
static uint8_t mIirReadCount;
static uint8_t mPrivateIoMarkerPrinted;


static EFI_STATUS EFIAPI AmiBackendIoRead(
    void *This,
    uint32_t Width,
    uint8_t BarIndex,
    uint64_t Offset,
    UINTN Count,
    void *Buffer
)
{
    uint8_t *Value;

    (void)This;
    if (
        Width != 0U ||
        BarIndex != 0U ||
        Offset > 7U ||
        Count != 1U ||
        Buffer == 0
    ) {
        return EFI_INVALID_PARAMETER;
    }
    if (mPrivateIoMarkerPrinted == 0U) {
        mPrivateIoMarkerPrinted = 1U;
        SerialWriteString("[NOMADIX-4FH] PRIVATE-IO\r\n");
    }
    Value = (uint8_t *)Buffer;
    if (Offset == 2U) {
        ++mIirReadCount;
        if (mIirReadCount == 2U) {
            *Value = 0xFFU;
            return EFI_SUCCESS;
        }
    }
    *Value = __inbyte((uint16_t)(COM1_BASE + (uint16_t)Offset));
    return EFI_SUCCESS;
}


static EFI_STATUS EFIAPI AmiBackendIoWrite(
    void *This,
    uint32_t Width,
    uint8_t BarIndex,
    uint64_t Offset,
    UINTN Count,
    void *Buffer
)
{
    (void)This;
    if (
        Width != 0U ||
        BarIndex != 0U ||
        Offset > 7U ||
        Count != 1U ||
        Buffer == 0
    ) {
        return EFI_INVALID_PARAMETER;
    }
    __outbyte(
        (uint16_t)(COM1_BASE + (uint16_t)Offset),
        *(const uint8_t *)Buffer
    );
    return EFI_SUCCESS;
}


static void PrepareAmiPrivateBackend(uint8_t *Container)
{
    mIirReadCount = 0;
    mPrivateIoMarkerPrinted = 0;
    *(AMI_PCI_IO_BACKEND **)
        (Container + AMI_SERIAL_PROTOCOL_BACKEND_OFFSET) =
            &mAmiPciIoBackend;
    Container[AMI_SERIAL_REGISTER_BAR_OFFSET] = 0;
    Container[AMI_SERIAL_ACCESS_WIDTH_OFFSET] = 0;
    Container[AMI_SERIAL_MMIO_FLAG_OFFSET] = 0;
}
