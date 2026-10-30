#ifndef NOMADIX_STAGE4FP_ENTRY
#define NOMADIX_STAGE4FP_ENTRY EfiMain
#endif
#ifndef NOMADIX_MAP_EXTRA_SETUP_GLYPH
#define NOMADIX_MAP_EXTRA_SETUP_GLYPH(Character, Ascii) \
    ((void)(Character), (void)(Ascii))
#endif
#include <stdint.h>

static uint64_t WriteSetupCharacter(uint16_t Character);
static int AcceptSetupKey(uint16_t ScanCode, uint16_t UnicodeChar);

#define NOMADIX_WRITE_CHARACTER(Character) \
    WriteSetupCharacter(Character)
#define NOMADIX_ACCEPT_KEY(ScanCode, UnicodeChar) \
    AcceptSetupKey((ScanCode), (UnicodeChar))
#define NOMADIX_STAGE4FO_ENTRY Stage4FOEfiMain
#include "../stage4fo/nomadix_serial_escape_scan_dxe.c"
#define EfiMain NOMADIX_STAGE4FP_ENTRY


static uint8_t mEscapeLatched;

static EFI_STATUS WriteSetupCharacter(uint16_t Character)
{
    uint8_t Ascii;

    Ascii = 0;
    if (
        Character == 0x2500U ||
        Character == 0x2501U ||
        Character == 0x2550U
    ) {
        Ascii = '-';
    } else if (
        Character == 0x2502U ||
        Character == 0x2503U ||
        Character == 0x2551U
    ) {
        Ascii = '|';
    } else if (Character >= 0x2500U && Character <= 0x257FU) {
        Ascii = '+';
    } else if (Character >= 0x2580U && Character <= 0x259FU) {
        Ascii = '#';
    } else if (Character == 0x2190U) {
        Ascii = '<';
    } else if (Character == 0x2191U || Character == 0x25B2U) {
        Ascii = '^';
    } else if (Character == 0x2192U) {
        Ascii = '>';
    } else if (Character == 0x2193U || Character == 0x25BCU) {
        Ascii = 'v';
    }
    NOMADIX_MAP_EXTRA_SETUP_GLYPH(Character, Ascii);
    if (Ascii != 0U) {
        return TerminalWrite(&Ascii, 1U);
    }
    return WriteUtf8(Character);
}

static int AcceptSetupKey(uint16_t ScanCode, uint16_t UnicodeChar)
{
    if (ScanCode != SCAN_ESC || UnicodeChar != 0U) {
        mEscapeLatched = 0;
        return 1;
    }
    if (mEscapeLatched != 0U) {
        return 0;
    }
    mEscapeLatched = 1U;
    return 1;
}

EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;
    static const uint16_t ActiveMessage[] = {
        '\r', '\n',
        '[', 'N', 'O', 'M', 'A', 'D', 'I', 'X', '-', '4', 'F', 'P', ']',
        ' ', 'S', 'E', 'T', 'U', 'P', '-', 'C', 'O', 'N', 'S', 'O', 'L', 'E',
        '\r', '\n',
        0
    };

    Status = Stage4FOEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    return NOMADIX_STAGE_MESSAGE(ActiveMessage);
}
#undef EfiMain
