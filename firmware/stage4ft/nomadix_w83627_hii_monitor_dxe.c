#include <stdint.h>

#ifndef NOMADIX_STAGE4FT_ENTRY
#define NOMADIX_STAGE4FT_ENTRY EfiMain
#endif
#ifndef NOMADIX_STAGE4FT_MARKER_STAGE
#define NOMADIX_STAGE4FT_MARKER_STAGE 'T'
#endif
#ifndef NOMADIX_STAGE4FT_DIAGNOSTICS
#define NOMADIX_STAGE4FT_DIAGNOSTICS 0
#endif

#define NOMADIX_STAGE4FS_ENTRY Stage4FSEfiMain
#include "../stage4fs/nomadix_serial_bold_attribute_dxe.c"

#define EFI_BUFFER_TOO_SMALL EFIERR(5)
#define EFI_HII_PACKAGE_TYPE_ALL 0x00U
#define HII_HANDLE_CAPACITY 256U
#define HII_TITLE_STRING_ID 0x05B7U
#define HII_PWM_PROMPT_STRING_ID 0x05BBU
#define HII_PWM_VALUE_STRING_ID 0x05BCU
#define HII_TEMP_PROMPT_STRING_ID 0x05D6U
#define HII_TEMP_VALUE_STRING_ID 0x05D7U
#define HII_FAN12_PROMPT_STRING_ID 0x05DCU
#define HII_FAN12_VALUE_STRING_ID 0x05DDU
#define HII_FAN34_PROMPT_STRING_ID 0x05DEU
#define HII_FAN34_VALUE_STRING_ID 0x05DFU
#define W83627_BANK_REGISTER 0x4EU

typedef void *EFI_HII_HANDLE;
typedef uint16_t EFI_STRING_ID;
typedef uint16_t LEGACY_HII_HANDLE;
typedef uint16_t LEGACY_STRING_REF;

typedef struct EFI_HII_STRING_PROTOCOL EFI_HII_STRING_PROTOCOL;
typedef struct EFI_HII_DATABASE_PROTOCOL EFI_HII_DATABASE_PROTOCOL;
typedef struct LEGACY_HII_PROTOCOL LEGACY_HII_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(
    const EFI_GUID *Protocol,
    void *Registration,
    void **Interface
);
typedef EFI_STATUS (EFIAPI *EFI_HII_GET_STRING)(
    const EFI_HII_STRING_PROTOCOL *This,
    const char *Language,
    EFI_HII_HANDLE PackageList,
    EFI_STRING_ID StringId,
    uint16_t *String,
    UINTN *StringSize,
    void **StringFontInfo
);
typedef EFI_STATUS (EFIAPI *EFI_HII_SET_STRING)(
    const EFI_HII_STRING_PROTOCOL *This,
    EFI_HII_HANDLE PackageList,
    EFI_STRING_ID StringId,
    const char *Language,
    uint16_t *String,
    const void *StringFontInfo
);
typedef EFI_STATUS (EFIAPI *EFI_HII_GET_LANGUAGES)(
    const EFI_HII_STRING_PROTOCOL *This,
    EFI_HII_HANDLE PackageList,
    char *Languages,
    UINTN *LanguagesSize
);
typedef EFI_STATUS (EFIAPI *EFI_HII_DATABASE_LIST_PACKS)(
    const EFI_HII_DATABASE_PROTOCOL *This,
    uint8_t PackageType,
    const EFI_GUID *PackageGuid,
    UINTN *HandleBufferLength,
    EFI_HII_HANDLE *Handle
);
typedef EFI_STATUS (EFIAPI *LEGACY_HII_FIND_HANDLES)(
    LEGACY_HII_PROTOCOL *This,
    uint16_t *HandleBufferLength,
    LEGACY_HII_HANDLE *Handle
);
typedef EFI_STATUS (EFIAPI *LEGACY_HII_NEW_STRING)(
    LEGACY_HII_PROTOCOL *This,
    uint16_t *Language,
    LEGACY_HII_HANDLE Handle,
    LEGACY_STRING_REF *Reference,
    uint16_t *NewString
);
typedef EFI_STATUS (EFIAPI *LEGACY_HII_GET_PRI_LANGUAGES)(
    LEGACY_HII_PROTOCOL *This,
    LEGACY_HII_HANDLE Handle,
    uint16_t **LanguageString
);
typedef EFI_STATUS (EFIAPI *LEGACY_HII_GET_STRING)(
    LEGACY_HII_PROTOCOL *This,
    LEGACY_HII_HANDLE Handle,
    LEGACY_STRING_REF Token,
    uint8_t Raw,
    uint16_t *LanguageString,
    UINTN *BufferLength,
    uint16_t *StringBuffer
);

struct EFI_HII_STRING_PROTOCOL {
    void *NewString;
    EFI_HII_GET_STRING GetString;
    EFI_HII_SET_STRING SetString;
    EFI_HII_GET_LANGUAGES GetLanguages;
    void *GetSecondaryLanguages;
};

struct EFI_HII_DATABASE_PROTOCOL {
    void *NewPackageList;
    void *RemovePackageList;
    void *UpdatePackageList;
    EFI_HII_DATABASE_LIST_PACKS ListPackageLists;
};

struct LEGACY_HII_PROTOCOL {
    void *NewPack;
    void *RemovePack;
    LEGACY_HII_FIND_HANDLES FindHandles;
    void *ExportDatabase;
    void *TestString;
    void *GetGlyph;
    void *GlyphToBlt;
    LEGACY_HII_NEW_STRING NewString;
    LEGACY_HII_GET_PRI_LANGUAGES GetPrimaryLanguages;
    void *GetSecondaryLanguages;
    LEGACY_HII_GET_STRING GetString;
    void *ResetStrings;
    void *GetLine;
    void *GetForms;
    void *GetDefaultImage;
    void *UpdateForm;
    void *GetKeyboardLayout;
};

static const EFI_GUID mHiiStringProtocolGuid = {
    0x0FD96974,
    0x23AA,
    0x4CDC,
    { 0xB9, 0xCB, 0x98, 0xD1, 0x77, 0x50, 0x32, 0x2A }
};
static const EFI_GUID mHiiDatabaseProtocolGuid = {
    0xEF9FC172,
    0xA1B2,
    0x4693,
    { 0xB3, 0x27, 0x6D, 0x32, 0xFC, 0x41, 0x60, 0x42 }
};
static const EFI_GUID mLegacyHiiProtocolGuid = {
    0xD7AD636E,
    0xB997,
    0x459B,
    { 0xBF, 0x3F, 0x88, 0x46, 0x89, 0x79, 0x80, 0xE1 }
};

static EFI_HII_STRING_PROTOCOL *mHiiString;
static EFI_HII_DATABASE_PROTOCOL *mHiiDatabase;
static LEGACY_HII_PROTOCOL *mLegacyHii;
static EFI_HII_HANDLE mSetupHiiHandle;
static LEGACY_HII_HANDLE mLegacySetupHiiHandle;
static EFI_EVENT mHwMonitorEvent;
static char mSetupLanguage[64];
static uint16_t mLegacyLanguage[4];
static uint8_t mStaticStringsUpdated;
static uint8_t mLegacyProtocolChecked;

#if NOMADIX_STAGE4FT_DIAGNOSTICS
static uint8_t mProtocolsReported;
static uint8_t mHandleScanReported;
static uint8_t mStringUpdateReported;
static uint8_t mLegacyProtocolReported;

static void TraceHiiStatus(
    const char *Operation,
    EFI_STATUS Status,
    UINTN Value
)
{
    SerialWriteString("\r\n[NOMADIX-4FU] ");
    SerialWriteString(Operation);
    SerialWriteString(" STATUS=");
    SerialWriteHex64(Status);
    SerialWriteString(" VALUE=");
    SerialWriteHex64(Value);
    SerialWriteString("\r\n");
}
#endif

static EFI_LOCATE_PROTOCOL GetLocateProtocol(void)
{
    EFI_BOOT_SERVICES_WITH_CRC *BootServices;

    BootServices = (EFI_BOOT_SERVICES_WITH_CRC *)mBootServices;
    return (EFI_LOCATE_PROTOCOL)BootServices->LocateProtocol;
}

static uint8_t W83627Read(uint16_t Register)
{
    __outbyte(HWM_INDEX, W83627_BANK_REGISTER);
    __outbyte(HWM_DATA, (uint8_t)(Register >> 8));
    __outbyte(HWM_INDEX, (uint8_t)Register);
    return __inbyte(HWM_DATA);
}

static uint16_t W83627ReadWord(uint16_t Register)
{
    uint16_t Value;

    Value = (uint16_t)W83627Read(Register) << 8;
    Value |= W83627Read((uint16_t)(Register + 1U));
    return Value;
}

static uint32_t FanRpm(uint8_t Count, uint8_t Divisor)
{
    if (Count == 0U || Count == 0xFFU) {
        return 0U;
    }
    return 1350000U / ((uint32_t)Count << Divisor);
}

static void ReadFanDivisors(uint8_t Divisors[4])
{
    uint8_t Div1;
    uint8_t Div2;
    uint8_t Diode;
    uint8_t SmiOvt;
    uint8_t Vbat;

    Div1 = W83627Read(0x0047U);
    Div2 = W83627Read(0x004BU);
    Vbat = W83627Read(0x005DU);
    Diode = W83627Read(0x0059U);
    SmiOvt = W83627Read(0x004CU);
    Divisors[0] = (uint8_t)(((Div1 >> 4) & 0x03U) | ((Vbat >> 3) & 0x04U));
    Divisors[1] = (uint8_t)(((Div1 >> 6) & 0x03U) | ((Vbat >> 4) & 0x04U));
    Divisors[2] = (uint8_t)(((Div2 >> 6) & 0x03U) | ((Vbat >> 5) & 0x04U));
    Divisors[3] = (uint8_t)((Diode & 0x03U) | ((SmiOvt >> 5) & 0x04U));
}

static UINTN AppendAscii(uint16_t *Buffer, UINTN Position, const char *Text)
{
    while (*Text != '\0') {
        Buffer[Position++] = (uint8_t)*Text++;
    }
    Buffer[Position] = 0;
    return Position;
}

static UINTN AppendUnsigned(
    uint16_t *Buffer,
    UINTN Position,
    uint32_t Value
)
{
    uint16_t Digits[10];
    UINTN Count;

    Count = 0;
    do {
        Digits[Count++] = (uint16_t)('0' + (Value % 10U));
        Value /= 10U;
    } while (Value != 0U);
    while (Count != 0U) {
        Buffer[Position++] = Digits[--Count];
    }
    Buffer[Position] = 0;
    return Position;
}

static UINTN AppendHalfDegrees(
    uint16_t *Buffer,
    UINTN Position,
    int32_t HalfDegrees
)
{
    uint32_t Magnitude;

    if (HalfDegrees < 0) {
        Buffer[Position++] = '-';
        Magnitude = (uint32_t)(-HalfDegrees);
    } else {
        Magnitude = (uint32_t)HalfDegrees;
    }
    Position = AppendUnsigned(Buffer, Position, Magnitude / 2U);
    Buffer[Position++] = '.';
    Buffer[Position++] = (Magnitude & 1U) != 0U ? '5' : '0';
    Buffer[Position] = 0;
    return Position;
}

static int UnicodeEqualsAscii(const uint16_t *Unicode, const char *Ascii)
{
    while (*Unicode != 0U && *Ascii != '\0') {
        if (*Unicode++ != (uint8_t)*Ascii++) {
            return 0;
        }
    }
    return *Unicode == 0U && *Ascii == '\0';
}

static int CopyNextLanguage(
    const char **Languages,
    char *Language,
    UINTN Capacity
)
{
    UINTN Length;

    while (**Languages == ';') {
        ++*Languages;
    }
    if (**Languages == '\0') {
        return 0;
    }
    Length = 0;
    while (
        **Languages != '\0' &&
        **Languages != ';' &&
        Length + 1U < Capacity
    ) {
        Language[Length++] = **Languages;
        ++*Languages;
    }
    Language[Length] = '\0';
    while (**Languages != '\0' && **Languages != ';') {
        ++*Languages;
    }
    return Length != 0U;
}

static EFI_STATUS LocateSetupHiiHandle(void)
{
    EFI_HII_HANDLE Handles[HII_HANDLE_CAPACITY];
    char Language[64];
    char Languages[256];
    uint16_t Title[64];
    UINTN BufferSize;
    UINTN HandleBufferSize;
    UINTN Index;
    UINTN LanguagesSize;
    const char *NextLanguage;
    EFI_STATUS Status;

    HandleBufferSize = sizeof(Handles);
    Status = mHiiDatabase->ListPackageLists(
        mHiiDatabase,
        EFI_HII_PACKAGE_TYPE_ALL,
        0,
        &HandleBufferSize,
        Handles
    );
#if NOMADIX_STAGE4FT_DIAGNOSTICS
    if (mHandleScanReported == 0U) {
        TraceHiiStatus(
            "LIST-PACKAGES",
            Status,
            HandleBufferSize / sizeof(Handles[0])
        );
        mHandleScanReported = 1U;
    }
#endif
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    for (Index = 0; Index < HandleBufferSize / sizeof(Handles[0]); ++Index) {
        LanguagesSize = sizeof(Languages);
        Status = mHiiString->GetLanguages(
            mHiiString,
            Handles[Index],
            Languages,
            &LanguagesSize
        );
        if (Status != EFI_SUCCESS) {
            continue;
        }
        NextLanguage = Languages;
        while (
            CopyNextLanguage(
                &NextLanguage,
                Language,
                sizeof(Language)
            )
        ) {
            BufferSize = sizeof(Title);
            Status = mHiiString->GetString(
                mHiiString,
                Language,
                Handles[Index],
                HII_TITLE_STRING_ID,
                Title,
                &BufferSize,
                0
            );
            if (
                Status == EFI_SUCCESS &&
                (
                    UnicodeEqualsAscii(Title, "IT8772E_B HW Monitor") ||
                    UnicodeEqualsAscii(
                        Title,
                        "W83627DHG-P Hardware Monitor"
                    )
                )
            ) {
                UINTN LanguageIndex;

                mSetupHiiHandle = Handles[Index];
                for (
                    LanguageIndex = 0;
                    LanguageIndex + 1U < sizeof(mSetupLanguage) &&
                    Language[LanguageIndex] != '\0';
                    ++LanguageIndex
                ) {
                    mSetupLanguage[LanguageIndex] = Language[LanguageIndex];
                }
                mSetupLanguage[LanguageIndex] = '\0';
#if NOMADIX_STAGE4FT_DIAGNOSTICS
                SerialWriteString("\r\n[NOMADIX-4FU] HII-HANDLE LANGUAGE=");
                SerialWriteString(mSetupLanguage);
                SerialWriteString("\r\n");
#endif
                return EFI_SUCCESS;
            }
        }
    }
    return EFI_NOT_FOUND;
}

static EFI_STATUS LocateLegacySetupHiiHandle(void)
{
    LEGACY_HII_HANDLE Handles[HII_HANDLE_CAPACITY];
    uint16_t DefaultLanguage[] = { 0 };
    uint16_t Title[64];
    uint16_t HandleBufferSize;
    uint16_t *Languages;
    UINTN BufferSize;
    UINTN Index;
    EFI_STATUS Status;

    HandleBufferSize = (uint16_t)sizeof(Handles);
    Status = mLegacyHii->FindHandles(
        mLegacyHii,
        &HandleBufferSize,
        Handles
    );
#if NOMADIX_STAGE4FT_DIAGNOSTICS
    TraceHiiStatus(
        "LEGACY-HANDLES",
        Status,
        HandleBufferSize / sizeof(Handles[0])
    );
#endif
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    for (Index = 0; Index < HandleBufferSize / sizeof(Handles[0]); ++Index) {
        BufferSize = sizeof(Title);
        Status = mLegacyHii->GetString(
            mLegacyHii,
            Handles[Index],
            HII_TITLE_STRING_ID,
            0,
            DefaultLanguage,
            &BufferSize,
            Title
        );
        if (
            Status == EFI_SUCCESS &&
            (
                UnicodeEqualsAscii(Title, "IT8772E_B HW Monitor") ||
                UnicodeEqualsAscii(
                    Title,
                    "W83627DHG-P Hardware Monitor"
                )
            )
        ) {
            mLegacySetupHiiHandle = Handles[Index];
            Languages = 0;
            Status = mLegacyHii->GetPrimaryLanguages(
                mLegacyHii,
                mLegacySetupHiiHandle,
                &Languages
            );
            if (Status != EFI_SUCCESS || Languages == 0) {
                return Status != EFI_SUCCESS ? Status : EFI_NOT_FOUND;
            }
            mLegacyLanguage[0] = Languages[0];
            mLegacyLanguage[1] = Languages[1];
            mLegacyLanguage[2] = Languages[2];
            mLegacyLanguage[3] = 0;
            mBootServices->FreePool(Languages);
#if NOMADIX_STAGE4FT_DIAGNOSTICS
            TraceHiiStatus(
                "LEGACY-HANDLE",
                EFI_SUCCESS,
                mLegacySetupHiiHandle
            );
#endif
            return EFI_SUCCESS;
        }
    }
    return EFI_NOT_FOUND;
}

static EFI_STATUS SetSetupString(EFI_STRING_ID StringId, uint16_t *String)
{
    LEGACY_STRING_REF Reference;
    EFI_STATUS Status;

    Status = mHiiString->SetString(
        mHiiString,
        mSetupHiiHandle,
        StringId,
        mSetupLanguage,
        String,
        0
    );
    if (Status != EFI_SUCCESS || mLegacySetupHiiHandle == 0U) {
        return Status;
    }
    Reference = StringId;
    return mLegacyHii->NewString(
        mLegacyHii,
        mLegacyLanguage,
        mLegacySetupHiiHandle,
        &Reference,
        String
    );
}

#if NOMADIX_STAGE4FT_DIAGNOSTICS
static void TraceTitleReadback(void)
{
    uint16_t Title[64];
    UINTN BufferSize;
    EFI_STATUS Status;
    UINTN Matches;

    BufferSize = sizeof(Title);
    Status = mHiiString->GetString(
        mHiiString,
        mSetupLanguage,
        mSetupHiiHandle,
        HII_TITLE_STRING_ID,
        Title,
        &BufferSize,
        0
    );
    Matches = (
        Status == EFI_SUCCESS &&
        UnicodeEqualsAscii(Title, "W83627DHG-P Hardware Monitor")
    ) ? 1U : 0U;
    TraceHiiStatus("GET-TITLE", Status, Matches);
    if (mLegacySetupHiiHandle != 0U) {
        uint16_t DefaultLanguage[] = { 0 };

        BufferSize = sizeof(Title);
        Status = mLegacyHii->GetString(
            mLegacyHii,
            mLegacySetupHiiHandle,
            HII_TITLE_STRING_ID,
            0,
            DefaultLanguage,
            &BufferSize,
            Title
        );
        Matches = (
            Status == EFI_SUCCESS &&
            UnicodeEqualsAscii(Title, "W83627DHG-P Hardware Monitor")
        ) ? 1U : 0U;
        TraceHiiStatus("LEGACY-GET-TITLE", Status, Matches);
    }
}
#endif

static EFI_STATUS UpdateStaticSetupStrings(void)
{
    static uint16_t Title[] = {
        'W', '8', '3', '6', '2', '7', 'D', 'H', 'G', '-', 'P', ' ',
        'H', 'a', 'r', 'd', 'w', 'a', 'r', 'e', ' ', 'M', 'o', 'n', 'i',
        't', 'o', 'r', 0
    };
    static uint16_t PwmPrompt[] = {
        'P', 'W', 'M', ' ', '1', '/', '2', '/', '3', '/', '4', 0
    };
    static uint16_t TempPrompt[] = {
        'B', 'o', 'a', 'r', 'd', ' ', 't', 'e', 'm', 'p', 'e', 'r', 'a',
        't', 'u', 'r', 'e', 's', 0
    };
    static uint16_t Fan12Prompt[] = {
        'F', 'a', 'n', ' ', '1', ' ', '/', ' ', 'F', 'a', 'n', ' ', '2', 0
    };
    static uint16_t Fan34Prompt[] = {
        'F', 'a', 'n', ' ', '3', ' ', '/', ' ', 'F', 'a', 'n', ' ', '4', 0
    };
    EFI_STATUS Status;

    Status = SetSetupString(HII_TITLE_STRING_ID, Title);
    if (Status == EFI_SUCCESS) {
        Status = SetSetupString(HII_PWM_PROMPT_STRING_ID, PwmPrompt);
    }
    if (Status == EFI_SUCCESS) {
        Status = SetSetupString(HII_TEMP_PROMPT_STRING_ID, TempPrompt);
    }
    if (Status == EFI_SUCCESS) {
        Status = SetSetupString(HII_FAN12_PROMPT_STRING_ID, Fan12Prompt);
    }
    if (Status == EFI_SUCCESS) {
        Status = SetSetupString(HII_FAN34_PROMPT_STRING_ID, Fan34Prompt);
    }
    return Status;
}

static EFI_STATUS UpdateSensorStrings(void)
{
    uint16_t Buffer[64];
    uint8_t Divisors[4];
    uint32_t Fan[4];
    int32_t Temp1;
    int32_t Temp3;
    UINTN Position;
    UINTN Index;
    EFI_STATUS Status;

    Temp1 = (int8_t)W83627Read(0x0027U) * 2;
    Temp3 = (int16_t)W83627ReadWord(0x0250U) / 128;
    ReadFanDivisors(Divisors);
    Fan[0] = FanRpm(W83627Read(0x0028U), Divisors[0]);
    Fan[1] = FanRpm(W83627Read(0x0029U), Divisors[1]);
    Fan[2] = FanRpm(W83627Read(0x002AU), Divisors[2]);
    Fan[3] = FanRpm(W83627Read(0x003FU), Divisors[3]);

    Position = 0;
    Position = AppendHalfDegrees(Buffer, Position, Temp1);
    Position = AppendAscii(Buffer, Position, " C / ");
    Position = AppendHalfDegrees(Buffer, Position, Temp3);
    AppendAscii(Buffer, Position, " C");
    Status = SetSetupString(HII_TEMP_VALUE_STRING_ID, Buffer);

    Position = 0;
    Position = AppendUnsigned(Buffer, Position, Fan[0]);
    Position = AppendAscii(Buffer, Position, " / ");
    Position = AppendUnsigned(Buffer, Position, Fan[1]);
    AppendAscii(Buffer, Position, " RPM");
    if (Status == EFI_SUCCESS) {
        Status = SetSetupString(HII_FAN12_VALUE_STRING_ID, Buffer);
    }

    Position = 0;
    Position = AppendUnsigned(Buffer, Position, Fan[2]);
    Position = AppendAscii(Buffer, Position, " / ");
    Position = AppendUnsigned(Buffer, Position, Fan[3]);
    AppendAscii(Buffer, Position, " RPM");
    if (Status == EFI_SUCCESS) {
        Status = SetSetupString(HII_FAN34_VALUE_STRING_ID, Buffer);
    }

    Position = 0;
    for (Index = 0; Index < 4U; ++Index) {
        uint8_t Pwm;

        Pwm = W83627Read(
            Index == 0U ? 0x0001U :
            Index == 1U ? 0x0003U :
            Index == 2U ? 0x0011U : 0x0061U
        );
        Position = AppendUnsigned(
            Buffer,
            Position,
            ((uint32_t)Pwm * 100U + 127U) / 255U
        );
        Position = AppendAscii(Buffer, Position, Index != 3U ? "% / " : "%");
    }
    if (Status == EFI_SUCCESS) {
        Status = SetSetupString(HII_PWM_VALUE_STRING_ID, Buffer);
    }
    return Status;
}

static EFI_STATUS LocateHiiProtocols(void)
{
    EFI_LOCATE_PROTOCOL LocateProtocol;
    EFI_STATUS Status;

    LocateProtocol = GetLocateProtocol();
    Status = LocateProtocol(
        &mHiiStringProtocolGuid,
        0,
        (void **)&mHiiString
    );
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    return LocateProtocol(
        &mHiiDatabaseProtocolGuid,
        0,
        (void **)&mHiiDatabase
    );
}

static EFI_STATUS LocateLegacyHiiProtocol(void)
{
    return GetLocateProtocol()(
        &mLegacyHiiProtocolGuid,
        0,
        (void **)&mLegacyHii
    );
}

static void EFIAPI UpdateHwMonitor(EFI_EVENT Event, void *Context)
{
    EFI_STATUS Status;

    (void)Event;
    (void)Context;
    if (mHiiString == 0 || mHiiDatabase == 0) {
        Status = LocateHiiProtocols();
#if NOMADIX_STAGE4FT_DIAGNOSTICS
        if (Status == EFI_SUCCESS && mProtocolsReported == 0U) {
            TraceHiiStatus("HII-PROTOCOLS", Status, 0);
            mProtocolsReported = 1U;
        }
#endif
        if (Status != EFI_SUCCESS) {
            return;
        }
    }
    if (mSetupHiiHandle == 0) {
        Status = LocateSetupHiiHandle();
        if (Status != EFI_SUCCESS) {
            return;
        }
    }
    if (mStaticStringsUpdated == 0U) {
        Status = UpdateStaticSetupStrings();
#if NOMADIX_STAGE4FT_DIAGNOSTICS
        if (mStringUpdateReported == 0U) {
            TraceHiiStatus("SET-STATIC", Status, 0);
            TraceTitleReadback();
        }
#endif
        if (Status != EFI_SUCCESS) {
            mSetupHiiHandle = 0;
            return;
        }
        mStaticStringsUpdated = 1U;
        GetSetTimer()(mHwMonitorEvent, TIMER_PERIODIC, 10000000U);
    }
    if (mLegacyProtocolChecked == 0U) {
        Status = LocateLegacyHiiProtocol();
#if NOMADIX_STAGE4FT_DIAGNOSTICS
        if (mLegacyProtocolReported == 0U) {
            TraceHiiStatus("LEGACY-PROTOCOL", Status, 0);
            mLegacyProtocolReported = 1U;
        }
#endif
        mLegacyProtocolChecked = 1U;
        if (Status == EFI_SUCCESS) {
            Status = LocateLegacySetupHiiHandle();
            if (Status == EFI_SUCCESS) {
                Status = UpdateStaticSetupStrings();
#if NOMADIX_STAGE4FT_DIAGNOSTICS
                TraceHiiStatus("LEGACY-SET-STATIC", Status, 0);
                TraceTitleReadback();
#endif
            }
        }
    }
    Status = UpdateSensorStrings();
#if NOMADIX_STAGE4FT_DIAGNOSTICS
    if (mStringUpdateReported == 0U) {
        TraceHiiStatus("SET-SENSORS", Status, 0);
        mStringUpdateReported = 1U;
    }
#endif
    if (Status != EFI_SUCCESS) {
        mSetupHiiHandle = 0;
        mLegacySetupHiiHandle = 0;
    }
}

EFI_STATUS EFIAPI NOMADIX_STAGE4FT_ENTRY(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;
    static const uint16_t ArmedMessage[] = {
        '\r', '\n',
        '[', 'N', 'O', 'M', 'A', 'D', 'I', 'X', '-', '4', 'F',
        NOMADIX_STAGE4FT_MARKER_STAGE, ']',
        ' ', 'W', '8', '3', '6', '2', '7', '-', 'H', 'I', 'I', '-',
        'A', 'R', 'M', 'E', 'D',
        '\r', '\n',
        0
    };

    Status = Stage4FSEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    mHwMonitorEvent = 0;
    Status = mBootServices->CreateEvent(
        EVT_TIMER | EVT_NOTIFY_SIGNAL,
        TPL_CALLBACK,
        UpdateHwMonitor,
        0,
        &mHwMonitorEvent
    );
    if (Status == EFI_SUCCESS) {
        Status = GetSetTimer()(mHwMonitorEvent, TIMER_PERIODIC, 100000U);
    }
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    UpdateHwMonitor(mHwMonitorEvent, 0);
    return SerialTextOutputString(&mSerialTextOut, ArmedMessage);
}
