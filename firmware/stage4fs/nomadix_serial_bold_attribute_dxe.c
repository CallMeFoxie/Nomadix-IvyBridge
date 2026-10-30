#include <stdint.h>

#ifndef NOMADIX_STAGE4FS_ENTRY
#define NOMADIX_STAGE4FS_ENTRY EfiMain
#endif

static uint64_t WriteAnsiIntensity(uint64_t Foreground);

#define NOMADIX_WRITE_ANSI_INTENSITY(Foreground) \
    WriteAnsiIntensity(Foreground)
#define NOMADIX_ANSI_FOREGROUND_BASE(Foreground) \
    ((void)(Foreground), 30U)
#define NOMADIX_STAGE4FR_ENTRY Stage4FREfiMain
#include "../stage4fr/nomadix_serial_ansi_input_fix_dxe.c"


static EFI_STATUS WriteAnsiIntensity(UINTN Foreground)
{
    return TerminalWriteLiteral(
        (Foreground & 0x08U) != 0U ? "1;" : "22;"
    );
}

EFI_STATUS EFIAPI NOMADIX_STAGE4FS_ENTRY(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;
    static const uint16_t ActiveMessage[] = {
        '\r', '\n',
        '[', 'N', 'O', 'M', 'A', 'D', 'I', 'X', '-', '4', 'F', 'S', ']',
        ' ', 'A', 'N', 'S', 'I', '-', 'B', 'O', 'L', 'D',
        '\r', '\n',
        0
    };

    Status = Stage4FREfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    return NOMADIX_STAGE_MESSAGE(ActiveMessage);
}
