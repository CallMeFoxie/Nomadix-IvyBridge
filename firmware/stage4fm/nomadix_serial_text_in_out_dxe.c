#ifndef NOMADIX_STAGE4FM_ENTRY
#define NOMADIX_STAGE4FM_ENTRY EfiMain
#endif
#ifndef NOMADIX_SERIAL_RX_BYTE
#define NOMADIX_SERIAL_RX_BYTE(Byte) ((void)(Byte))
#endif
#ifndef NOMADIX_KEY_QUEUED
#define NOMADIX_KEY_QUEUED(ScanCode, UnicodeChar) \
    ((void)(ScanCode), (void)(UnicodeChar))
#endif
#ifndef NOMADIX_QUEUE_ESCAPE
#define NOMADIX_QUEUE_ESCAPE() QueueUnicode(0x001BU)
#endif
#ifndef NOMADIX_ACCEPT_KEY
#define NOMADIX_ACCEPT_KEY(ScanCode, UnicodeChar) \
    ((void)(ScanCode), (void)(UnicodeChar), 1)
#endif
#ifndef NOMADIX_AGE_ESCAPE_ON_POLL
#define NOMADIX_AGE_ESCAPE_ON_POLL(Event) \
    ((void)(Event), 1)
#endif
#define NOMADIX_STAGE4FL_ENTRY Stage4FLEfiMain
#include "../stage4fl/nomadix_serial_text_out_dxe.c"
#define EfiMain NOMADIX_STAGE4FM_ENTRY


#define EFI_NOT_READY EFIERR(6)

#define EVT_TIMER 0x80000000U
#define EVT_NOTIFY_WAIT 0x00000100U
#define EVT_NOTIFY_SIGNAL 0x00000200U
#define TIMER_CANCEL 0U
#define TIMER_PERIODIC 1U
#define TPL_NOTIFY 16U

#define SCAN_NULL 0x0000U
#define SCAN_UP 0x0001U
#define SCAN_DOWN 0x0002U
#define SCAN_RIGHT 0x0003U
#define SCAN_LEFT 0x0004U
#define SCAN_HOME 0x0005U
#define SCAN_END 0x0006U
#define SCAN_INSERT 0x0007U
#define SCAN_DELETE 0x0008U
#define SCAN_PAGE_UP 0x0009U
#define SCAN_PAGE_DOWN 0x000AU
#define SCAN_F1 0x000BU
#define SCAN_F2 0x000CU
#define SCAN_F3 0x000DU
#define SCAN_F4 0x000EU
#define SCAN_F5 0x000FU
#define SCAN_F6 0x0010U
#define SCAN_F7 0x0011U
#define SCAN_F8 0x0012U
#define SCAN_F9 0x0013U
#define SCAN_F10 0x0014U
#define SCAN_F11 0x0015U
#define SCAN_F12 0x0016U

#define KEY_QUEUE_CAPACITY 32U
#define ESCAPE_BUFFER_CAPACITY 8U
#define ESCAPE_TIMEOUT_TICKS 3U

typedef struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef struct {
    uint16_t ScanCode;
    uint16_t UnicodeChar;
} EFI_INPUT_KEY;

typedef EFI_STATUS (EFIAPI *EFI_INPUT_RESET)(
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    uint8_t ExtendedVerification
);
typedef EFI_STATUS (EFIAPI *EFI_INPUT_READ_KEY)(
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    EFI_INPUT_KEY *Key
);
typedef EFI_STATUS (EFIAPI *EFI_SET_TIMER)(
    EFI_EVENT Event,
    uint32_t Type,
    uint64_t TriggerTime
);
typedef EFI_STATUS (EFIAPI *EFI_SIGNAL_EVENT)(EFI_EVENT Event);
typedef EFI_STATUS (EFIAPI *EFI_CLOSE_EVENT)(EFI_EVENT Event);
typedef EFI_TPL (EFIAPI *EFI_RAISE_TPL)(EFI_TPL NewTpl);
typedef void (EFIAPI *EFI_RESTORE_TPL)(EFI_TPL OldTpl);
typedef EFI_STATUS (EFIAPI *EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES)(
    EFI_HANDLE Handle,
    ...
);

struct EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    EFI_INPUT_RESET Reset;
    EFI_INPUT_READ_KEY ReadKeyStroke;
    EFI_EVENT WaitForKey;
};

typedef char AssertInputKeySize[
    sizeof(EFI_INPUT_KEY) == 4 ? 1 : -1
];
typedef char AssertTextInProtocolSize[
    sizeof(EFI_SIMPLE_TEXT_INPUT_PROTOCOL) == 24 ? 1 : -1
];

static const EFI_GUID mSimpleTextInProtocolGuid = {
    0x387477C1,
    0x69C7,
    0x11D2,
    {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}
};

static EFI_STATUS EFIAPI SerialInputReset(
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    uint8_t ExtendedVerification
);
static EFI_STATUS EFIAPI SerialInputReadKey(
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    EFI_INPUT_KEY *Key
);
static void EFIAPI PollSerialInput(EFI_EVENT Event, void *Context);
static void ProcessInputByte(uint8_t Byte);

static EFI_SIMPLE_TEXT_INPUT_PROTOCOL mSerialTextIn = {
    SerialInputReset,
    SerialInputReadKey,
    0
};

static EFI_INPUT_KEY mKeyQueue[KEY_QUEUE_CAPACITY];
static uint8_t mKeyReadIndex;
static uint8_t mKeyWriteIndex;
static uint8_t mKeyCount;
static uint8_t mEscapeBuffer[ESCAPE_BUFFER_CAPACITY];
static uint8_t mEscapeLength;
static uint8_t mEscapeAge;
static uint8_t mLastWasCarriageReturn;
static EFI_EVENT mInputPollEvent;

static EFI_SET_TIMER GetSetTimer(void)
{
    return (EFI_SET_TIMER)mBootServices->SetTimer;
}

static EFI_SIGNAL_EVENT GetSignalEvent(void)
{
    return (EFI_SIGNAL_EVENT)mBootServices->SignalEvent;
}

static EFI_CLOSE_EVENT GetCloseEvent(void)
{
    return (EFI_CLOSE_EVENT)mBootServices->CloseEvent;
}

static EFI_RAISE_TPL GetRaiseTpl(void)
{
    return (EFI_RAISE_TPL)mBootServices->RaiseTPL;
}

static EFI_RESTORE_TPL GetRestoreTpl(void)
{
    return (EFI_RESTORE_TPL)mBootServices->RestoreTPL;
}

static void SignalWaitForKey(void)
{
    if (mSerialTextIn.WaitForKey != 0) {
        GetSignalEvent()(mSerialTextIn.WaitForKey);
    }
}

static void QueueKey(uint16_t ScanCode, uint16_t UnicodeChar)
{
    if (
        NOMADIX_ACCEPT_KEY(ScanCode, UnicodeChar) == 0 ||
        mKeyCount >= KEY_QUEUE_CAPACITY
    ) {
        return;
    }
    mKeyQueue[mKeyWriteIndex].ScanCode = ScanCode;
    mKeyQueue[mKeyWriteIndex].UnicodeChar = UnicodeChar;
    NOMADIX_KEY_QUEUED(ScanCode, UnicodeChar);
    mKeyWriteIndex = (uint8_t)(
        (mKeyWriteIndex + 1U) % KEY_QUEUE_CAPACITY
    );
    ++mKeyCount;
    SignalWaitForKey();
}

static void QueueUnicode(uint16_t Character)
{
    QueueKey(SCAN_NULL, Character);
}

static void QueueScan(uint16_t ScanCode)
{
    QueueKey(ScanCode, 0);
}

static int CompleteCsiNumber(uint32_t Number)
{
    switch (Number) {
        case 1U:
            QueueScan(SCAN_HOME);
            break;
        case 2U:
            QueueScan(SCAN_INSERT);
            break;
        case 3U:
            QueueScan(SCAN_DELETE);
            break;
        case 4U:
            QueueScan(SCAN_END);
            break;
        case 5U:
            QueueScan(SCAN_PAGE_UP);
            break;
        case 6U:
            QueueScan(SCAN_PAGE_DOWN);
            break;
        case 7U:
            QueueScan(SCAN_HOME);
            break;
        case 8U:
            QueueScan(SCAN_END);
            break;
        case 11U:
            QueueScan(SCAN_F1);
            break;
        case 12U:
            QueueScan(SCAN_F2);
            break;
        case 13U:
            QueueScan(SCAN_F3);
            break;
        case 14U:
            QueueScan(SCAN_F4);
            break;
        case 15U:
            QueueScan(SCAN_F5);
            break;
        case 17U:
            QueueScan(SCAN_F6);
            break;
        case 18U:
            QueueScan(SCAN_F7);
            break;
        case 19U:
            QueueScan(SCAN_F8);
            break;
        case 20U:
            QueueScan(SCAN_F9);
            break;
        case 21U:
            QueueScan(SCAN_F10);
            break;
        case 23U:
            QueueScan(SCAN_F11);
            break;
        case 24U:
            QueueScan(SCAN_F12);
            break;
        default:
            return 0;
    }
    return 1;
}

static int TryCompleteEscape(void)
{
    uint8_t Final;
    uint8_t Index;
    uint32_t Number;

    if (mEscapeLength < 2U) {
        return 0;
    }
    if (mEscapeBuffer[1] == 'O') {
        if (mEscapeLength < 3U) {
            return 0;
        }
        if (mEscapeLength != 3U) {
            return -1;
        }
        switch (mEscapeBuffer[2]) {
            case 'A':
                QueueScan(SCAN_UP);
                return 1;
            case 'B':
                QueueScan(SCAN_DOWN);
                return 1;
            case 'C':
                QueueScan(SCAN_RIGHT);
                return 1;
            case 'D':
                QueueScan(SCAN_LEFT);
                return 1;
            case 'H':
                QueueScan(SCAN_HOME);
                return 1;
            case 'F':
                QueueScan(SCAN_END);
                return 1;
            case 'P':
                QueueScan(SCAN_F1);
                return 1;
            case 'Q':
                QueueScan(SCAN_F2);
                return 1;
            case 'R':
                QueueScan(SCAN_F3);
                return 1;
            case 'S':
                QueueScan(SCAN_F4);
                return 1;
            default:
                return -1;
        }
    }
    if (mEscapeBuffer[1] != '[') {
        return -1;
    }
    if (mEscapeLength < 3U) {
        return 0;
    }
    Final = mEscapeBuffer[mEscapeLength - 1U];
    if (mEscapeLength == 3U) {
        switch (Final) {
            case 'A':
                QueueScan(SCAN_UP);
                return 1;
            case 'B':
                QueueScan(SCAN_DOWN);
                return 1;
            case 'C':
                QueueScan(SCAN_RIGHT);
                return 1;
            case 'D':
                QueueScan(SCAN_LEFT);
                return 1;
            case 'H':
                QueueScan(SCAN_HOME);
                return 1;
            case 'F':
                QueueScan(SCAN_END);
                return 1;
            default:
                break;
        }
    }
    if (Final == '~') {
        if (mEscapeLength < 4U) {
            return -1;
        }
        Number = 0;
        for (Index = 2U; Index + 1U < mEscapeLength; ++Index) {
            if (
                mEscapeBuffer[Index] < '0' ||
                mEscapeBuffer[Index] > '9'
            ) {
                return -1;
            }
            Number = Number * 10U +
                     (uint32_t)(mEscapeBuffer[Index] - '0');
        }
        return CompleteCsiNumber(Number) != 0 ? 1 : -1;
    }
    for (Index = 2U; Index < mEscapeLength; ++Index) {
        if (
            mEscapeBuffer[Index] < '0' ||
            mEscapeBuffer[Index] > '9'
        ) {
            return -1;
        }
    }
    return mEscapeLength < ESCAPE_BUFFER_CAPACITY ? 0 : -1;
}

static void FlushEscapeAsLiteral(void)
{
    uint8_t Byte;
    uint8_t Index;
    uint8_t Length;

    Length = mEscapeLength;
    mEscapeLength = 0;
    mEscapeAge = 0;
    NOMADIX_QUEUE_ESCAPE();
    for (Index = 1U; Index < Length; ++Index) {
        Byte = mEscapeBuffer[Index];
        ProcessInputByte(Byte);
    }
}

static void ProcessAsciiByte(uint8_t Byte)
{
    if (Byte == 0x0DU) {
        QueueUnicode(0x000DU);
        mLastWasCarriageReturn = 1U;
        return;
    }
    if (Byte == 0x0AU) {
        if (mLastWasCarriageReturn == 0U) {
            QueueUnicode(0x000DU);
        }
        mLastWasCarriageReturn = 0U;
        return;
    }
    mLastWasCarriageReturn = 0U;
    if (Byte == 0x7FU) {
        QueueScan(SCAN_DELETE);
    } else if (Byte != 0U) {
        QueueUnicode((uint16_t)Byte);
    }
}

static void ProcessInputByte(uint8_t Byte)
{
    int Result;

    NOMADIX_SERIAL_RX_BYTE(Byte);
    if (mEscapeLength == 0U) {
        if (Byte == 0x1BU) {
            mEscapeBuffer[0] = Byte;
            mEscapeLength = 1U;
            mEscapeAge = 0U;
        } else {
            ProcessAsciiByte(Byte);
        }
        return;
    }
    if (mEscapeLength >= ESCAPE_BUFFER_CAPACITY) {
        FlushEscapeAsLiteral();
        ProcessInputByte(Byte);
        return;
    }
    mEscapeBuffer[mEscapeLength++] = Byte;
    mEscapeAge = 0U;
    Result = TryCompleteEscape();
    if (Result > 0) {
        mEscapeLength = 0;
        mEscapeAge = 0;
    } else if (Result < 0) {
        FlushEscapeAsLiteral();
    }
}

static void EFIAPI PollSerialInput(EFI_EVENT Event, void *Context)
{
    uint32_t Count;
    uint8_t Received;

    (void)Event;
    (void)Context;
    Received = 0;
    for (Count = 0; Count < 64U; ++Count) {
        if ((__inbyte(COM1_BASE + 5) & UART_LSR_DATA_READY) == 0U) {
            break;
        }
        ProcessInputByte(__inbyte(COM1_BASE));
        Received = 1U;
    }
    if (mEscapeLength != 0U) {
        if (
            Received == 0U &&
            NOMADIX_AGE_ESCAPE_ON_POLL(Event) != 0
        ) {
            ++mEscapeAge;
        }
        if (mEscapeAge >= ESCAPE_TIMEOUT_TICKS) {
            FlushEscapeAsLiteral();
        }
    }
    if (mKeyCount != 0U) {
        SignalWaitForKey();
    }
}

static EFI_STATUS EFIAPI SerialInputReset(
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    uint8_t ExtendedVerification
)
{
    EFI_TPL OldTpl;

    (void)ExtendedVerification;
    if (This != &mSerialTextIn) {
        return EFI_INVALID_PARAMETER;
    }
    OldTpl = GetRaiseTpl()(TPL_NOTIFY);
    mKeyReadIndex = 0;
    mKeyWriteIndex = 0;
    mKeyCount = 0;
    mEscapeLength = 0;
    mEscapeAge = 0;
    mLastWasCarriageReturn = 0;
    while ((__inbyte(COM1_BASE + 5) & UART_LSR_DATA_READY) != 0U) {
        (void)__inbyte(COM1_BASE);
    }
    GetRestoreTpl()(OldTpl);
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI SerialInputReadKey(
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    EFI_INPUT_KEY *Key
)
{
    EFI_TPL OldTpl;

    if (This != &mSerialTextIn || Key == 0) {
        return EFI_INVALID_PARAMETER;
    }
    PollSerialInput(0, 0);
    OldTpl = GetRaiseTpl()(TPL_NOTIFY);
    if (mKeyCount == 0U) {
        GetRestoreTpl()(OldTpl);
        return EFI_NOT_READY;
    }
    *Key = mKeyQueue[mKeyReadIndex];
    mKeyReadIndex = (uint8_t)(
        (mKeyReadIndex + 1U) % KEY_QUEUE_CAPACITY
    );
    --mKeyCount;
    if (mKeyCount != 0U) {
        SignalWaitForKey();
    }
    GetRestoreTpl()(OldTpl);
    return EFI_SUCCESS;
}

static void CleanupInputEvents(void)
{
    if (mInputPollEvent != 0) {
        GetSetTimer()(mInputPollEvent, TIMER_CANCEL, 0);
        GetCloseEvent()(mInputPollEvent);
        mInputPollEvent = 0;
    }
    if (mSerialTextIn.WaitForKey != 0) {
        GetCloseEvent()(mSerialTextIn.WaitForKey);
        mSerialTextIn.WaitForKey = 0;
    }
}

static EFI_STATUS CreateInputEvents(void)
{
    EFI_STATUS Status;

    Status = mBootServices->CreateEvent(
        EVT_NOTIFY_WAIT,
        TPL_CALLBACK,
        PollSerialInput,
        0,
        &mSerialTextIn.WaitForKey
    );
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = mBootServices->CreateEvent(
        EVT_TIMER | EVT_NOTIFY_SIGNAL,
        TPL_CALLBACK,
        PollSerialInput,
        0,
        &mInputPollEvent
    );
    if (Status != EFI_SUCCESS) {
        CleanupInputEvents();
        return Status;
    }
    Status = GetSetTimer()(
        mInputPollEvent,
        TIMER_PERIODIC,
        100000U
    );
    if (Status != EFI_SUCCESS) {
        CleanupInputEvents();
    }
    return Status;
}

static EFI_STATUS ActivateSerialInput(
    EFI_SYSTEM_TABLE *SystemTable,
    EFI_BOOT_SERVICES_WITH_CRC *BootServices
)
{
    EFI_HANDLE OriginalConsoleInHandle;
    EFI_STATUS Status;
    uint32_t OriginalCrc32;
    void *OriginalConIn;

    OriginalConsoleInHandle = SystemTable->ConsoleInHandle;
    OriginalConIn = SystemTable->ConIn;
    OriginalCrc32 = SystemTable->Hdr.CRC32;
    SystemTable->ConsoleInHandle = mCom1Handle;
    SystemTable->ConIn = &mSerialTextIn;
    SystemTable->Hdr.CRC32 = 0;
    Status = BootServices->CalculateCrc32(
        SystemTable,
        SystemTable->Hdr.HeaderSize,
        &SystemTable->Hdr.CRC32
    );
    if (Status != EFI_SUCCESS) {
        SystemTable->ConsoleInHandle = OriginalConsoleInHandle;
        SystemTable->ConIn = OriginalConIn;
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
    EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES Uninstall;
    static const uint16_t ActiveMessage[] = {
        '\r', '\n',
        '[', 'N', 'O', 'M', 'A', 'D', 'I', 'X', '-', '4', 'F', 'M', ']',
        ' ', 'C', 'O', 'N', 'I', 'N', '-', 'A', 'C', 'T', 'I', 'V', 'E',
        '\r', '\n',
        0
    };

    Status = Stage4FLEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    BootServices = (EFI_BOOT_SERVICES_WITH_CRC *)SystemTable->BootServices;
    Status = CreateInputEvents();
    SerialWriteString("[NOMADIX-4FM] EVENTS=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    Status = BootServices->InstallMultipleProtocolInterfaces(
        &mCom1Handle,
        &mSimpleTextInProtocolGuid,
        &mSerialTextIn,
        0
    );
    SerialWriteString("[NOMADIX-4FM] TEXT-IN=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        CleanupInputEvents();
        return Status;
    }
    Status = ActivateSerialInput(SystemTable, BootServices);
    SerialWriteString("[NOMADIX-4FM] CONIN=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        Uninstall = (EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES)
            BootServices->UninstallMultipleProtocolInterfaces;
        Uninstall(
            mCom1Handle,
            &mSimpleTextInProtocolGuid,
            &mSerialTextIn,
            0
        );
        CleanupInputEvents();
        return Status;
    }
    return NOMADIX_STAGE_MESSAGE(ActiveMessage);
}
#undef EfiMain
