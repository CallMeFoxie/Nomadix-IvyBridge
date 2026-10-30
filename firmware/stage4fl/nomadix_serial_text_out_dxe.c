#ifndef NOMADIX_STAGE4FL_ENTRY
#define NOMADIX_STAGE4FL_ENTRY EfiMain
#endif
#ifndef NOMADIX_WRITE_CHARACTER
#define NOMADIX_WRITE_CHARACTER(Character) WriteUtf8(Character)
#endif
#ifndef NOMADIX_WRITE_ANSI_INTENSITY
#define NOMADIX_WRITE_ANSI_INTENSITY(Foreground) \
    ((void)(Foreground), EFI_SUCCESS)
#endif
#ifndef NOMADIX_ANSI_FOREGROUND_BASE
#define NOMADIX_ANSI_FOREGROUND_BASE(Foreground) \
    (((Foreground) & 0x08U) != 0U ? 90U : 30U)
#endif
#define NOMADIX_STAGE4F4_ENTRY Stage4F4EfiMain
#include "../stage4f4/nomadix_serial_io_dxe.c"
#define EfiMain NOMADIX_STAGE4FL_ENTRY


#define EFI_WARN_UNKNOWN_GLYPH 1

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_TEXT_RESET)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    uint8_t ExtendedVerification
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_STRING)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    const uint16_t *String
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_QUERY_MODE)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN ModeNumber,
    UINTN *Columns,
    UINTN *Rows
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_MODE)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN ModeNumber
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN Attribute
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_CLEAR_SCREEN)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_CURSOR_POSITION)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN Column,
    UINTN Row
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_ENABLE_CURSOR)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    uint8_t Visible
);
typedef EFI_STATUS (EFIAPI *EFI_CALCULATE_CRC32)(
    void *Data,
    UINTN DataSize,
    uint32_t *Crc32
);

typedef struct {
    int32_t MaxMode;
    int32_t Mode;
    int32_t Attribute;
    int32_t CursorColumn;
    int32_t CursorRow;
    uint8_t CursorVisible;
} EFI_SIMPLE_TEXT_OUTPUT_MODE;

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_TEXT_RESET Reset;
    EFI_TEXT_STRING OutputString;
    EFI_TEXT_STRING TestString;
    EFI_TEXT_QUERY_MODE QueryMode;
    EFI_TEXT_SET_MODE SetMode;
    EFI_TEXT_SET_ATTRIBUTE SetAttribute;
    EFI_TEXT_CLEAR_SCREEN ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;
    EFI_TEXT_ENABLE_CURSOR EnableCursor;
    EFI_SIMPLE_TEXT_OUTPUT_MODE *Mode;
};

typedef struct {
    EFI_BOOT_SERVICES Base;
    void *LocateProtocol;
    EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES
        InstallMultipleProtocolInterfaces;
    void *UninstallMultipleProtocolInterfaces;
    EFI_CALCULATE_CRC32 CalculateCrc32;
} EFI_BOOT_SERVICES_WITH_CRC;

typedef char AssertTextOutModeSize[
    sizeof(EFI_SIMPLE_TEXT_OUTPUT_MODE) == 24 ? 1 : -1
];
typedef char AssertTextOutProtocolSize[
    sizeof(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL) == 80 ? 1 : -1
];
typedef char AssertCalculateCrc32Offset[
    offsetof(EFI_BOOT_SERVICES_WITH_CRC, CalculateCrc32) == 344 ? 1 : -1
];

static EFI_STATUS EFIAPI SerialTextReset(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    uint8_t ExtendedVerification
);
static EFI_STATUS EFIAPI SerialTextOutputString(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    const uint16_t *String
);
static EFI_STATUS EFIAPI SerialTextTestString(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    const uint16_t *String
);
static EFI_STATUS EFIAPI SerialTextQueryMode(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN ModeNumber,
    UINTN *Columns,
    UINTN *Rows
);
static EFI_STATUS EFIAPI SerialTextSetMode(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN ModeNumber
);
static EFI_STATUS EFIAPI SerialTextSetAttribute(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN Attribute
);
static EFI_STATUS EFIAPI SerialTextClearScreen(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
);
static EFI_STATUS EFIAPI SerialTextSetCursorPosition(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN Column,
    UINTN Row
);
static EFI_STATUS EFIAPI SerialTextEnableCursor(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    uint8_t Visible
);

static EFI_SIMPLE_TEXT_OUTPUT_MODE mSerialTextMode = {
    1,
    0,
    0x07,
    0,
    0,
    1
};

static EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL mSerialTextOut = {
    SerialTextReset,
    SerialTextOutputString,
    SerialTextTestString,
    SerialTextQueryMode,
    SerialTextSetMode,
    SerialTextSetAttribute,
    SerialTextClearScreen,
    SerialTextSetCursorPosition,
    SerialTextEnableCursor,
    &mSerialTextMode
};

static EFI_STATUS TerminalWrite(const uint8_t *Bytes, UINTN Size)
{
    UINTN Written;

    Written = Size;
    return mCom1Protocol.Write(
        &mCom1Protocol,
        &Written,
        (void *)Bytes
    );
}

#if NOMADIX_ENABLE_DIAGNOSTIC_LOG
#define NOMADIX_STAGE_MESSAGE(Message) \
    SerialTextOutputString(&mSerialTextOut, (Message))
#else
#define NOMADIX_STAGE_MESSAGE(Message) ((void)(Message), EFI_SUCCESS)
#endif

static EFI_STATUS TerminalWriteLiteral(const char *String)
{
    const char *End;

    End = String;
    while (*End != '\0') {
        ++End;
    }
    return TerminalWrite(
        (const uint8_t *)String,
        (UINTN)(End - String)
    );
}

static EFI_STATUS TerminalWriteDecimal(UINTN Value)
{
    uint8_t Buffer[20];
    UINTN Index;
    UINTN Start;

    Index = sizeof(Buffer);
    do {
        Buffer[--Index] = (uint8_t)('0' + Value % 10U);
        Value /= 10U;
    } while (Value != 0U);
    Start = Index;
    return TerminalWrite(Buffer + Start, sizeof(Buffer) - Start);
}

static EFI_STATUS WriteUtf8(uint16_t Character)
{
    uint8_t Bytes[3];

    if (Character < 0x80U) {
        Bytes[0] = (uint8_t)Character;
        return TerminalWrite(Bytes, 1U);
    }
    if (Character < 0x800U) {
        Bytes[0] = (uint8_t)(0xC0U | (Character >> 6));
        Bytes[1] = (uint8_t)(0x80U | (Character & 0x3FU));
        return TerminalWrite(Bytes, 2U);
    }
    Bytes[0] = (uint8_t)(0xE0U | (Character >> 12));
    Bytes[1] = (uint8_t)(0x80U | ((Character >> 6) & 0x3FU));
    Bytes[2] = (uint8_t)(0x80U | (Character & 0x3FU));
    return TerminalWrite(Bytes, 3U);
}

static void UpdateCursor(uint16_t Character)
{
    if (Character == 0x08U) {
        if (mSerialTextMode.CursorColumn > 0) {
            --mSerialTextMode.CursorColumn;
        }
        return;
    }
    if (Character == 0x0DU) {
        mSerialTextMode.CursorColumn = 0;
        return;
    }
    if (Character == 0x0AU) {
        if (mSerialTextMode.CursorRow < 24) {
            ++mSerialTextMode.CursorRow;
        }
        return;
    }
    if (Character < 0x20U) {
        return;
    }
    ++mSerialTextMode.CursorColumn;
    if (mSerialTextMode.CursorColumn >= 80) {
        mSerialTextMode.CursorColumn = 0;
        if (mSerialTextMode.CursorRow < 24) {
            ++mSerialTextMode.CursorRow;
        }
    }
}

static EFI_STATUS EFIAPI SerialTextReset(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    uint8_t ExtendedVerification
)
{
    EFI_STATUS Status;

    (void)ExtendedVerification;
    if (This != &mSerialTextOut) {
        return EFI_INVALID_PARAMETER;
    }
    mSerialTextMode.Mode = 0;
    mSerialTextMode.Attribute = 0x07;
    mSerialTextMode.CursorColumn = 0;
    mSerialTextMode.CursorRow = 0;
    mSerialTextMode.CursorVisible = 1;
    Status = mCom1Protocol.Reset(&mCom1Protocol);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    return TerminalWriteLiteral("\x1B[0m\x1B[2J\x1B[H\x1B[?25h");
}

static EFI_STATUS EFIAPI SerialTextOutputString(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    const uint16_t *String
)
{
    EFI_STATUS Status;
    uint8_t UnknownGlyph;

    if (This != &mSerialTextOut || String == 0) {
        return EFI_INVALID_PARAMETER;
    }
    UnknownGlyph = 0;
    while (*String != 0U) {
        if (*String >= 0xD800U && *String <= 0xDFFFU) {
            Status = TerminalWriteLiteral("?");
            UnknownGlyph = 1;
        } else {
            Status = NOMADIX_WRITE_CHARACTER(*String);
        }
        if (Status != EFI_SUCCESS) {
            return Status;
        }
        UpdateCursor(*String);
        ++String;
    }
    return UnknownGlyph != 0U ? EFI_WARN_UNKNOWN_GLYPH : EFI_SUCCESS;
}

static EFI_STATUS EFIAPI SerialTextTestString(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    const uint16_t *String
)
{
    if (This != &mSerialTextOut || String == 0) {
        return EFI_INVALID_PARAMETER;
    }
    while (*String != 0U) {
        if (*String >= 0xD800U && *String <= 0xDFFFU) {
            return EFI_UNSUPPORTED;
        }
        ++String;
    }
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI SerialTextQueryMode(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN ModeNumber,
    UINTN *Columns,
    UINTN *Rows
)
{
    if (
        This != &mSerialTextOut ||
        Columns == 0 ||
        Rows == 0
    ) {
        return EFI_INVALID_PARAMETER;
    }
    if (ModeNumber != 0U) {
        return EFI_UNSUPPORTED;
    }
    *Columns = 80U;
    *Rows = 25U;
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI SerialTextSetMode(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN ModeNumber
)
{
    if (This != &mSerialTextOut) {
        return EFI_INVALID_PARAMETER;
    }
    if (ModeNumber != 0U) {
        return EFI_UNSUPPORTED;
    }
    mSerialTextMode.Mode = 0;
    return SerialTextClearScreen(This);
}

static EFI_STATUS EFIAPI SerialTextSetAttribute(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN Attribute
)
{
    static const uint8_t ColorMap[8] = {
        0, 4, 2, 6, 1, 5, 3, 7
    };
    EFI_STATUS Status;
    UINTN Background;
    UINTN Foreground;

    if (This != &mSerialTextOut || (Attribute & ~0x7FU) != 0U) {
        return EFI_INVALID_PARAMETER;
    }
    Foreground = Attribute & 0x0FU;
    Background = (Attribute >> 4) & 0x07U;
    Status = TerminalWriteLiteral("\x1B[");
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = NOMADIX_WRITE_ANSI_INTENSITY(Foreground);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = TerminalWriteDecimal(
        NOMADIX_ANSI_FOREGROUND_BASE(Foreground) +
        ColorMap[Foreground & 0x07U]
    );
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = TerminalWriteLiteral(";");
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = TerminalWriteDecimal(40U + ColorMap[Background]);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = TerminalWriteLiteral("m");
    if (Status == EFI_SUCCESS) {
        mSerialTextMode.Attribute = (int32_t)Attribute;
    }
    return Status;
}

static EFI_STATUS EFIAPI SerialTextClearScreen(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
)
{
    EFI_STATUS Status;

    if (This != &mSerialTextOut) {
        return EFI_INVALID_PARAMETER;
    }
    Status = TerminalWriteLiteral("\x1B[2J\x1B[H");
    if (Status == EFI_SUCCESS) {
        mSerialTextMode.CursorColumn = 0;
        mSerialTextMode.CursorRow = 0;
    }
    return Status;
}

static EFI_STATUS EFIAPI SerialTextSetCursorPosition(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    UINTN Column,
    UINTN Row
)
{
    EFI_STATUS Status;

    if (
        This != &mSerialTextOut ||
        Column >= 80U ||
        Row >= 25U
    ) {
        return EFI_INVALID_PARAMETER;
    }
    Status = TerminalWriteLiteral("\x1B[");
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = TerminalWriteDecimal(Row + 1U);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = TerminalWriteLiteral(";");
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = TerminalWriteDecimal(Column + 1U);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = TerminalWriteLiteral("H");
    if (Status == EFI_SUCCESS) {
        mSerialTextMode.CursorColumn = (int32_t)Column;
        mSerialTextMode.CursorRow = (int32_t)Row;
    }
    return Status;
}

static EFI_STATUS EFIAPI SerialTextEnableCursor(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    uint8_t Visible
)
{
    EFI_STATUS Status;

    if (This != &mSerialTextOut) {
        return EFI_INVALID_PARAMETER;
    }
    Status = TerminalWriteLiteral(
        Visible != 0U ? "\x1B[?25h" : "\x1B[?25l"
    );
    if (Status == EFI_SUCCESS) {
        mSerialTextMode.CursorVisible = Visible != 0U ? 1U : 0U;
    }
    return Status;
}

static EFI_STATUS ActivateSerialConsole(
    EFI_SYSTEM_TABLE *SystemTable,
    EFI_BOOT_SERVICES_WITH_CRC *BootServices
)
{
    EFI_HANDLE OriginalConsoleOutHandle;
    EFI_HANDLE OriginalStandardErrorHandle;
    EFI_STATUS Status;
    uint32_t OriginalCrc32;
    void *OriginalConOut;
    void *OriginalStdErr;

    OriginalConsoleOutHandle = SystemTable->ConsoleOutHandle;
    OriginalConOut = SystemTable->ConOut;
    OriginalStandardErrorHandle = SystemTable->StandardErrorHandle;
    OriginalStdErr = SystemTable->StdErr;
    OriginalCrc32 = SystemTable->Hdr.CRC32;

    SystemTable->ConsoleOutHandle = mCom1Handle;
    SystemTable->ConOut = &mSerialTextOut;
    SystemTable->StandardErrorHandle = mCom1Handle;
    SystemTable->StdErr = &mSerialTextOut;
    SystemTable->Hdr.CRC32 = 0;
    Status = BootServices->CalculateCrc32(
        SystemTable,
        SystemTable->Hdr.HeaderSize,
        &SystemTable->Hdr.CRC32
    );
    if (Status != EFI_SUCCESS) {
        SystemTable->ConsoleOutHandle = OriginalConsoleOutHandle;
        SystemTable->ConOut = OriginalConOut;
        SystemTable->StandardErrorHandle = OriginalStandardErrorHandle;
        SystemTable->StdErr = OriginalStdErr;
        SystemTable->Hdr.CRC32 = OriginalCrc32;
    }
    return Status;
}

EFI_STATUS EFIAPI EfiMain(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_BOOT_SERVICES_WITH_CRC *BootServices;
    EFI_STATUS Status;
    static const uint16_t ActiveMessage[] = {
        '\r', '\n',
        '[', 'N', 'O', 'M', 'A', 'D', 'I', 'X', '-', '4', 'F', 'L', ']',
        ' ', 'C', 'O', 'N', 'O', 'U', 'T', '-', 'A', 'C', 'T', 'I', 'V', 'E',
        '\r', '\n',
        0
    };

    Status = Stage4F4EfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    if (SystemTable == 0 || SystemTable->BootServices == 0) {
        return EFI_DEVICE_ERROR;
    }
    BootServices = (EFI_BOOT_SERVICES_WITH_CRC *)SystemTable->BootServices;
    Status = BootServices->InstallMultipleProtocolInterfaces(
        &mCom1Handle,
        &mSimpleTextOutProtocolGuid,
        &mSerialTextOut,
        0
    );
    SerialWriteString("[NOMADIX-4FL] TEXT-OUT=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = ActivateSerialConsole(SystemTable, BootServices);
    SerialWriteString("[NOMADIX-4FL] CONOUT=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = SerialTextReset(&mSerialTextOut, 0);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    return NOMADIX_STAGE_MESSAGE(ActiveMessage);
}
#undef EfiMain
