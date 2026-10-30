#ifndef NOMADIX_STAGE4FR_ENTRY
#define NOMADIX_STAGE4FR_ENTRY EfiMain
#endif
#define NOMADIX_AGE_ESCAPE_ON_POLL(Event) \
    ((Event) == mInputPollEvent)
#define NOMADIX_MAP_EXTRA_SETUP_GLYPH(Character, Ascii) \
    do { \
        if ((Character) == 0x25B6U || (Character) == 0x25BAU) { \
            (Ascii) = '>'; \
        } else if ((Character) == 0x25C0U || (Character) == 0x25C4U) { \
            (Ascii) = '<'; \
        } \
    } while (0)
#define NOMADIX_STAGE4FQ_ENTRY Stage4FQEfiMain
#include "../stage4fq/nomadix_serial_clean_console_dxe.c"
#define EfiMain NOMADIX_STAGE4FR_ENTRY


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;
    static const uint16_t ActiveMessage[] = {
        '\r', '\n',
        '[', 'N', 'O', 'M', 'A', 'D', 'I', 'X', '-', '4', 'F', 'R', ']',
        ' ', 'A', 'N', 'S', 'I', '-', 'I', 'N', 'P', 'U', 'T', '-', 'F', 'I', 'X',
        '\r', '\n',
        0
    };

    Status = Stage4FQEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    return NOMADIX_STAGE_MESSAGE(ActiveMessage);
}
#undef EfiMain
