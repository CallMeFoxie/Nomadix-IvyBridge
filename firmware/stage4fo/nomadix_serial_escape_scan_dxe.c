#ifndef NOMADIX_STAGE4FO_ENTRY
#define NOMADIX_STAGE4FO_ENTRY EfiMain
#endif
#define SCAN_ESC 0x0017U
#define NOMADIX_QUEUE_ESCAPE() QueueScan(SCAN_ESC)
#define NOMADIX_STAGE4FN_ENTRY Stage4FNEfiMain
#include "../stage4fn/nomadix_serial_text_input_ex_dxe.c"
#define EfiMain NOMADIX_STAGE4FO_ENTRY


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;
    static const uint16_t ActiveMessage[] = {
        '\r', '\n',
        '[', 'N', 'O', 'M', 'A', 'D', 'I', 'X', '-', '4', 'F', 'O', ']',
        ' ', 'E', 'S', 'C', '-', 'S', 'C', 'A', 'N', '=',
        '0', '0', '1', '7',
        '\r', '\n',
        0
    };

    Status = Stage4FNEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    return NOMADIX_STAGE_MESSAGE(ActiveMessage);
}
#undef EfiMain
