#ifndef NOMADIX_STAGE4F9_ENTRY
#define NOMADIX_STAGE4F9_ENTRY EfiMain
#endif

#define NOMADIX_TRACE_SET_ATTRIBUTES_INPUT( \
    BaudRate, ReceiveFifoDepth, Timeout, Parity, DataBits, StopBits \
) do { \
    SerialWriteString("[NOMADIX-4F9] SETATTR BAUD="); \
    SerialWriteHex64(BaudRate); \
    SerialWriteString(" FIFO="); \
    SerialWriteHex64(ReceiveFifoDepth); \
    SerialWriteString(" TIMEOUT="); \
    SerialWriteHex64(Timeout); \
    SerialWriteString(" PARITY="); \
    SerialWriteHex64(Parity); \
    SerialWriteString(" DATA="); \
    SerialWriteHex64(DataBits); \
    SerialWriteString(" STOP="); \
    SerialWriteHex64(StopBits); \
    SerialWriteString("\r\n"); \
} while (0)

#define NOMADIX_TRACE_SET_ATTRIBUTES_RESULT(Status) do { \
    SerialWriteString("[NOMADIX-4F9] SETATTR_STATUS="); \
    SerialWriteHex64(Status); \
    SerialWriteString("\r\n"); \
} while (0)

#define NOMADIX_TRACE_SET_CONTROL_INPUT(Control) do { \
    SerialWriteString("[NOMADIX-4F9] SETCONTROL CONTROL="); \
    SerialWriteHex64(Control); \
    SerialWriteString("\r\n"); \
} while (0)

#define NOMADIX_TRACE_SET_CONTROL_RESULT(Status) do { \
    SerialWriteString("[NOMADIX-4F9] SETCONTROL_STATUS="); \
    SerialWriteHex64(Status); \
    SerialWriteString("\r\n"); \
} while (0)

#define NOMADIX_STAGE4F8_ENTRY Stage4F8EfiMain
#include "../stage4f8/nomadix_terminal_private_bypass_dxe.c"
#undef NOMADIX_STAGE4F8_ENTRY
#define EfiMain NOMADIX_STAGE4F9_ENTRY


EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    return Stage4F8EfiMain(ImageHandle, SystemTable);
}
#undef EfiMain
