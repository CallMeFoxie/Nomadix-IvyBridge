#ifndef NOMADIX_STAGE4FQ_ENTRY
#define NOMADIX_STAGE4FQ_ENTRY EfiMain
#endif
#define NOMADIX_ENABLE_STAGE4F3_DIAGNOSTICS 0
#define NOMADIX_ENABLE_STAGE4F4_DIAGNOSTICS 0
#define NOMADIX_ENABLE_INPUT_TRACE 0
#define NOMADIX_STAGE4FP_ENTRY Stage4FPEfiMain
#include "../stage4fp/nomadix_serial_setup_console_dxe.c"
#define EfiMain NOMADIX_STAGE4FQ_ENTRY


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;
    static const uint16_t ActiveMessage[] = {
        '\r', '\n',
        '[', 'N', 'O', 'M', 'A', 'D', 'I', 'X', '-', '4', 'F', 'Q', ']',
        ' ', 'C', 'L', 'E', 'A', 'N', '-', 'C', 'O', 'N', 'S', 'O', 'L', 'E',
        '\r', '\n',
        0
    };

    Status = Stage4FPEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    return NOMADIX_STAGE_MESSAGE(ActiveMessage);
}
#undef EfiMain
