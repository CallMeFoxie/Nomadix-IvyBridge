#include <stdint.h>

static uint64_t PatchTerminalPreflightStatus(
    uint8_t *Image,
    uint64_t Size
);

#define NOMADIX_PATCH_TERMINAL_IMAGE(Image, Size) \
    PatchTerminalPreflightStatus((Image), (uint64_t)(Size))
#define NOMADIX_STAGE4FA_ENTRY Stage4FAEfiMain
#include "../stage4fa/nomadix_terminal_device_path_open_diag_dxe.c"
#undef NOMADIX_STAGE4FA_ENTRY


#define TERMINAL_PREFLIGHT_BRANCH_OFFSET 0x2C0AU

static const uint8_t mExpectedPreflightBranch[6] = {
    0x0F, 0x85, 0xAE, 0x06, 0x00, 0x00
};
static const uint8_t mPreservePreflightStatusBranch[6] = {
    0x0F, 0x85, 0xE8, 0x06, 0x00, 0x00
};


static uint64_t PatchTerminalPreflightStatus(
    uint8_t *Image,
    uint64_t Size
)
{
    UINTN Index;

    if (Image == 0 ||
        Size < TERMINAL_PREFLIGHT_BRANCH_OFFSET +
            sizeof(mExpectedPreflightBranch)) {
        return EFI_UNSUPPORTED;
    }
    for (Index = 0; Index < sizeof(mExpectedPreflightBranch); ++Index) {
        if (Image[TERMINAL_PREFLIGHT_BRANCH_OFFSET + Index] !=
            mExpectedPreflightBranch[Index]) {
            return EFI_UNSUPPORTED;
        }
    }
    for (Index = 0; Index < sizeof(mPreservePreflightStatusBranch); ++Index) {
        Image[TERMINAL_PREFLIGHT_BRANCH_OFFSET + Index] =
            mPreservePreflightStatusBranch[Index];
    }
    SerialWriteString("[NOMADIX-4FB] PREFLIGHT-STATUS-PATCHED\r\n");
    return EFI_SUCCESS;
}


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    SerialWriteString("[NOMADIX-4FB] ARMED\r\n");
    return Stage4FAEfiMain(ImageHandle, SystemTable);
}
