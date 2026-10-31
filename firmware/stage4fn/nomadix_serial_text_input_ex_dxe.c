#ifndef NOMADIX_STAGE4FN_ENTRY
#define NOMADIX_STAGE4FN_ENTRY EfiMain
#endif
#ifndef NOMADIX_ENABLE_INPUT_TRACE
#define NOMADIX_ENABLE_INPUT_TRACE 1
#endif
#include <stdint.h>

static void TraceSerialRxByte(uint8_t Byte);
static void SerialInputExKeyQueued(
    uint16_t ScanCode,
    uint16_t UnicodeChar
);

#define NOMADIX_STAGE4FM_ENTRY Stage4FMEfiMain
#define NOMADIX_SERIAL_RX_BYTE(Byte) TraceSerialRxByte(Byte)
#define NOMADIX_KEY_QUEUED(ScanCode, UnicodeChar) \
    SerialInputExKeyQueued((ScanCode), (UnicodeChar))
#include "../stage4fm/nomadix_serial_text_in_out_dxe.c"
#define EfiMain NOMADIX_STAGE4FN_ENTRY


#define EFI_OUT_OF_RESOURCES EFIERR(9)
#define EFI_NOT_FOUND EFIERR(14)
#define EFI_SHIFT_STATE_VALID 0x80000000U
#define EFI_TOGGLE_STATE_VALID 0x80U
#define EFI_SCROLL_LOCK_ACTIVE 0x01U
#define EFI_NUM_LOCK_ACTIVE 0x02U
#define EFI_CAPS_LOCK_ACTIVE 0x04U
#define KEY_NOTIFY_CAPACITY 16U

typedef struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL;

typedef uint8_t EFI_KEY_TOGGLE_STATE;

typedef struct {
    uint32_t KeyShiftState;
    EFI_KEY_TOGGLE_STATE KeyToggleState;
} EFI_KEY_STATE;

typedef struct {
    EFI_INPUT_KEY Key;
    EFI_KEY_STATE KeyState;
} EFI_KEY_DATA;

typedef EFI_STATUS (EFIAPI *EFI_INPUT_RESET_EX)(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    uint8_t ExtendedVerification
);
typedef EFI_STATUS (EFIAPI *EFI_INPUT_READ_KEY_EX)(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    EFI_KEY_DATA *KeyData
);
typedef EFI_STATUS (EFIAPI *EFI_SET_STATE)(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    EFI_KEY_TOGGLE_STATE *KeyToggleState
);
typedef EFI_STATUS (EFIAPI *EFI_KEY_NOTIFY_FUNCTION)(
    EFI_KEY_DATA *KeyData
);
typedef EFI_STATUS (EFIAPI *EFI_REGISTER_KEYSTROKE_NOTIFY)(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    EFI_KEY_DATA *KeyData,
    EFI_KEY_NOTIFY_FUNCTION KeyNotificationFunction,
    void **NotifyHandle
);
typedef EFI_STATUS (EFIAPI *EFI_UNREGISTER_KEYSTROKE_NOTIFY)(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    void *NotificationHandle
);

struct EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL {
    EFI_INPUT_RESET_EX Reset;
    EFI_INPUT_READ_KEY_EX ReadKeyStrokeEx;
    EFI_EVENT WaitForKeyEx;
    EFI_SET_STATE SetState;
    EFI_REGISTER_KEYSTROKE_NOTIFY RegisterKeyNotify;
    EFI_UNREGISTER_KEYSTROKE_NOTIFY UnregisterKeyNotify;
};

typedef struct {
    uint8_t InUse;
    EFI_KEY_DATA KeyData;
    EFI_KEY_NOTIFY_FUNCTION Function;
} NOMADIX_KEY_NOTIFY;

typedef char AssertKeyStateSize[
    sizeof(EFI_KEY_STATE) == 8 ? 1 : -1
];
typedef char AssertKeyDataSize[
    sizeof(EFI_KEY_DATA) == 12 ? 1 : -1
];
typedef char AssertTextInputExProtocolSize[
    sizeof(EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL) == 48 ? 1 : -1
];

static const EFI_GUID mSimpleTextInputExProtocolGuid = {
    0xDD9E7534,
    0x7762,
    0x4698,
    {0x8C, 0x14, 0xF5, 0x85, 0x17, 0xA6, 0x25, 0xAA}
};

static EFI_STATUS EFIAPI SerialInputExReset(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    uint8_t ExtendedVerification
);
static EFI_STATUS EFIAPI SerialInputExReadKey(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    EFI_KEY_DATA *KeyData
);
static EFI_STATUS EFIAPI SerialInputExSetState(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    EFI_KEY_TOGGLE_STATE *KeyToggleState
);
static EFI_STATUS EFIAPI SerialInputExRegisterNotify(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    EFI_KEY_DATA *KeyData,
    EFI_KEY_NOTIFY_FUNCTION KeyNotificationFunction,
    void **NotifyHandle
);
static EFI_STATUS EFIAPI SerialInputExUnregisterNotify(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    void *NotificationHandle
);

static EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL mSerialTextInputEx = {
    SerialInputExReset,
    SerialInputExReadKey,
    0,
    SerialInputExSetState,
    SerialInputExRegisterNotify,
    SerialInputExUnregisterNotify
};

static NOMADIX_KEY_NOTIFY mKeyNotifies[KEY_NOTIFY_CAPACITY];
static EFI_KEY_TOGGLE_STATE mKeyToggleState;
static uint8_t mRxTraced;
static uint8_t mKeyTraced;
static uint8_t mReadExTraced;
static uint8_t mRegisterTraced;

static EFI_KEY_DATA MakeKeyData(
    uint16_t ScanCode,
    uint16_t UnicodeChar
)
{
    EFI_KEY_DATA KeyData;

    KeyData.Key.ScanCode = ScanCode;
    KeyData.Key.UnicodeChar = UnicodeChar;
    KeyData.KeyState.KeyShiftState = EFI_SHIFT_STATE_VALID;
    KeyData.KeyState.KeyToggleState = (EFI_KEY_TOGGLE_STATE)(
        EFI_TOGGLE_STATE_VALID | mKeyToggleState
    );
    return KeyData;
}

static int KeyDataMatches(
    const EFI_KEY_DATA *Registered,
    const EFI_KEY_DATA *Current
)
{
    if (
        Registered->Key.ScanCode != Current->Key.ScanCode ||
        Registered->Key.UnicodeChar != Current->Key.UnicodeChar
    ) {
        return 0;
    }
    if (
        (Registered->KeyState.KeyShiftState &
         EFI_SHIFT_STATE_VALID) != 0U &&
        (Registered->KeyState.KeyShiftState &
         ~EFI_SHIFT_STATE_VALID) !=
        (Current->KeyState.KeyShiftState &
         ~EFI_SHIFT_STATE_VALID)
    ) {
        return 0;
    }
    if (
        (Registered->KeyState.KeyToggleState &
         EFI_TOGGLE_STATE_VALID) != 0U &&
        (Registered->KeyState.KeyToggleState &
         (EFI_SCROLL_LOCK_ACTIVE |
          EFI_NUM_LOCK_ACTIVE |
          EFI_CAPS_LOCK_ACTIVE)) !=
        (Current->KeyState.KeyToggleState &
         (EFI_SCROLL_LOCK_ACTIVE |
          EFI_NUM_LOCK_ACTIVE |
          EFI_CAPS_LOCK_ACTIVE))
    ) {
        return 0;
    }
    return 1;
}

static void TraceSerialRxByte(uint8_t Byte)
{
#if NOMADIX_ENABLE_INPUT_TRACE
    if (mRxTraced != 0U) {
        return;
    }
    mRxTraced = 1U;
    SerialWriteString("\r\n[NOMADIX-4FN] RX=");
    SerialWriteHex64(Byte);
    SerialWriteString("\r\n");
#else
    (void)Byte;
#endif
}

static void SerialInputExKeyQueued(
    uint16_t ScanCode,
    uint16_t UnicodeChar
)
{
    EFI_KEY_DATA KeyData;
    EFI_KEY_NOTIFY_FUNCTION Function;
    uint8_t Index;

    KeyData = MakeKeyData(ScanCode, UnicodeChar);
#if NOMADIX_ENABLE_INPUT_TRACE
    if (mKeyTraced == 0U) {
        mKeyTraced = 1U;
        SerialWriteString("[NOMADIX-4FN] KEY SCAN=");
        SerialWriteHex64(ScanCode);
        SerialWriteString(" CHAR=");
        SerialWriteHex64(UnicodeChar);
        SerialWriteString("\r\n");
    }
#endif
    for (Index = 0; Index < KEY_NOTIFY_CAPACITY; ++Index) {
        if (
            mKeyNotifies[Index].InUse != 0U &&
            KeyDataMatches(&mKeyNotifies[Index].KeyData, &KeyData)
        ) {
            Function = mKeyNotifies[Index].Function;
            Function(&KeyData);
        }
    }
}

static EFI_STATUS EFIAPI SerialInputExReset(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    uint8_t ExtendedVerification
)
{
    if (This != &mSerialTextInputEx) {
        return EFI_INVALID_PARAMETER;
    }
    return SerialInputReset(&mSerialTextIn, ExtendedVerification);
}

static EFI_STATUS EFIAPI SerialInputExReadKey(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    EFI_KEY_DATA *KeyData
)
{
    EFI_INPUT_KEY Key;
    EFI_STATUS Status;

    if (This != &mSerialTextInputEx || KeyData == 0) {
        return EFI_INVALID_PARAMETER;
    }
    Status = SerialInputReadKey(&mSerialTextIn, &Key);
#if NOMADIX_ENABLE_INPUT_TRACE
    if (mReadExTraced == 0U) {
        mReadExTraced = 1U;
        SerialWriteString("[NOMADIX-4FN] READ-EX=");
        SerialWriteHex64(Status);
        SerialWriteString("\r\n");
    }
#endif
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    *KeyData = MakeKeyData(Key.ScanCode, Key.UnicodeChar);
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI SerialInputExSetState(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    EFI_KEY_TOGGLE_STATE *KeyToggleState
)
{
    if (This != &mSerialTextInputEx || KeyToggleState == 0) {
        return EFI_INVALID_PARAMETER;
    }
    mKeyToggleState = (EFI_KEY_TOGGLE_STATE)(
        *KeyToggleState &
        (EFI_SCROLL_LOCK_ACTIVE |
         EFI_NUM_LOCK_ACTIVE |
         EFI_CAPS_LOCK_ACTIVE)
    );
    return EFI_SUCCESS;
}

static EFI_STATUS EFIAPI SerialInputExRegisterNotify(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    EFI_KEY_DATA *KeyData,
    EFI_KEY_NOTIFY_FUNCTION KeyNotificationFunction,
    void **NotifyHandle
)
{
    uint8_t Index;

    if (
        This != &mSerialTextInputEx ||
        KeyData == 0 ||
        KeyNotificationFunction == 0 ||
        NotifyHandle == 0
    ) {
        return EFI_INVALID_PARAMETER;
    }
#if NOMADIX_ENABLE_INPUT_TRACE
    if (mRegisterTraced == 0U) {
        mRegisterTraced = 1U;
        SerialWriteString("[NOMADIX-4FN] REGISTER-NOTIFY\r\n");
    }
#endif
    for (Index = 0; Index < KEY_NOTIFY_CAPACITY; ++Index) {
        if (
            mKeyNotifies[Index].InUse != 0U &&
            mKeyNotifies[Index].Function == KeyNotificationFunction &&
            KeyDataMatches(&mKeyNotifies[Index].KeyData, KeyData)
        ) {
            *NotifyHandle = &mKeyNotifies[Index];
            return EFI_SUCCESS;
        }
    }
    for (Index = 0; Index < KEY_NOTIFY_CAPACITY; ++Index) {
        if (mKeyNotifies[Index].InUse == 0U) {
            mKeyNotifies[Index].KeyData = *KeyData;
            mKeyNotifies[Index].Function = KeyNotificationFunction;
            mKeyNotifies[Index].InUse = 1U;
            *NotifyHandle = &mKeyNotifies[Index];
            return EFI_SUCCESS;
        }
    }
    return EFI_OUT_OF_RESOURCES;
}

static EFI_STATUS EFIAPI SerialInputExUnregisterNotify(
    EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
    void *NotificationHandle
)
{
    uint8_t Index;

    if (This != &mSerialTextInputEx || NotificationHandle == 0) {
        return EFI_INVALID_PARAMETER;
    }
    for (Index = 0; Index < KEY_NOTIFY_CAPACITY; ++Index) {
        if (
            NotificationHandle == &mKeyNotifies[Index] &&
            mKeyNotifies[Index].InUse != 0U
        ) {
            mKeyNotifies[Index].InUse = 0;
            mKeyNotifies[Index].Function = 0;
            return EFI_SUCCESS;
        }
    }
    return EFI_NOT_FOUND;
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
        '[', 'N', 'O', 'M', 'A', 'D', 'I', 'X', '-', '4', 'F', 'N', ']',
        ' ', 'T', 'E', 'X', 'T', '-', 'I', 'N', '-', 'E', 'X', '-',
        'A', 'C', 'T', 'I', 'V', 'E',
        '\r', '\n',
        0
    };

    Status = Stage4FMEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    mSerialTextInputEx.WaitForKeyEx = mSerialTextIn.WaitForKey;
    BootServices = (EFI_BOOT_SERVICES_WITH_CRC *)SystemTable->BootServices;
    Status = BootServices->InstallMultipleProtocolInterfaces(
        &mCom1Handle,
        &mSimpleTextInputExProtocolGuid,
        &mSerialTextInputEx,
        0
    );
    SerialWriteString("[NOMADIX-4FN] TEXT-IN-EX=");
    SerialWriteHex64(Status);
    SerialWriteString("\r\n");
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    return NOMADIX_STAGE_MESSAGE(ActiveMessage);
}
#undef EfiMain
