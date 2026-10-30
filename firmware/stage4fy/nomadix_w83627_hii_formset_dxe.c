#include <stdint.h>

#ifndef NOMADIX_STAGE4FY_ENTRY
#define NOMADIX_STAGE4FY_ENTRY EfiMain
#endif
#ifndef NOMADIX_STAGE4FY_MARKER_STAGE
#define NOMADIX_STAGE4FY_MARKER_STAGE 'Y'
#endif
#ifndef NOMADIX_STAGE4FY_MARKER_GROUP
#define NOMADIX_STAGE4FY_MARKER_GROUP 'F'
#endif
#ifndef NOMADIX_ENABLE_FAN_PROFILES
#define NOMADIX_ENABLE_FAN_PROFILES 0
#endif
#ifndef NOMADIX_PERSIST_FAN_PROFILE
#define NOMADIX_PERSIST_FAN_PROFILE 0
#endif

#define NOMADIX_STAGE4FS_ENTRY Stage4FSEfiMain
#include "../stage4fs/nomadix_serial_bold_attribute_dxe.c"

#define EFI_NOT_READY EFIERR(6)
#define HII_PACKAGE_CAPACITY 2048U
#define HII_FORM_ID 1U
#define HII_STRING_LANGUAGE_NAME 1U
#define HII_STRING_FORM_TITLE 2U
#define HII_STRING_FORM_HELP 3U
#define HII_STRING_CHIP_PROMPT 4U
#define HII_STRING_CHIP_VALUE 5U
#define HII_STRING_BASE_PROMPT 6U
#define HII_STRING_BASE_VALUE 7U
#define HII_STRING_TEMP_PROMPT 8U
#define HII_STRING_TEMP_VALUE 9U
#define HII_STRING_FAN12_PROMPT 10U
#define HII_STRING_FAN12_VALUE 11U
#define HII_STRING_FAN34_PROMPT 12U
#define HII_STRING_FAN34_VALUE 13U
#define HII_STRING_PWM_PROMPT 14U
#define HII_STRING_PWM_VALUE 15U
#if NOMADIX_ENABLE_FAN_PROFILES
#define HII_STRING_PROFILE_PROMPT 16U
#define HII_STRING_PROFILE_VALUE 17U
#define HII_STRING_QUIET_ACTION 18U
#define HII_STRING_QUIET_HELP 19U
#define HII_STRING_BALANCED_ACTION 20U
#define HII_STRING_BALANCED_HELP 21U
#define HII_STRING_FULL_ACTION 22U
#define HII_STRING_FULL_HELP 23U
#define QUESTION_QUIET 0x1001U
#define QUESTION_BALANCED 0x1002U
#define QUESTION_FULL 0x1003U
#define EFI_BROWSER_ACTION_CHANGING 0U
#define EFI_BROWSER_ACTION_CHANGED 1U
#define EFI_BROWSER_ACTION_REQUEST_NONE 0U
#define FAN_PROFILE_QUIET 0U
#define FAN_PROFILE_BALANCED 1U
#define FAN_PROFILE_FULL 2U
#if NOMADIX_PERSIST_FAN_PROFILE
#define EFI_NOT_FOUND EFIERR(14)
#define EFI_COMPROMISED_DATA EFIERR(33)
#define EFI_VARIABLE_NON_VOLATILE 0x00000001U
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002U
#define FAN_PROFILE_SIGNATURE 0x3150464EU
#define FAN_PROFILE_VERSION 1U
#endif
#endif
#define W83627_BANK_REGISTER 0x4EU

typedef void *EFI_HII_HANDLE;
typedef uint16_t EFI_STRING_ID;

typedef struct HII_DATABASE_PROTOCOL HII_DATABASE_PROTOCOL;
typedef struct HII_STRING_PROTOCOL HII_STRING_PROTOCOL;
#if NOMADIX_ENABLE_FAN_PROFILES
typedef struct HII_CONFIG_ACCESS_PROTOCOL HII_CONFIG_ACCESS_PROTOCOL;
#if NOMADIX_PERSIST_FAN_PROFILE
typedef EFI_STATUS (EFIAPI *EFI_GET_VARIABLE)(
    const uint16_t *VariableName,
    const EFI_GUID *VendorGuid,
    uint32_t *Attributes,
    UINTN *DataSize,
    void *Data
);
typedef EFI_STATUS (EFIAPI *EFI_SET_VARIABLE)(
    const uint16_t *VariableName,
    const EFI_GUID *VendorGuid,
    uint32_t Attributes,
    UINTN DataSize,
    const void *Data
);
#endif
#endif

typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(
    const EFI_GUID *Protocol,
    void *Registration,
    void **Interface
);
typedef EFI_STATUS (EFIAPI *HII_NEW_PACKAGE_LIST)(
    const HII_DATABASE_PROTOCOL *This,
    const void *PackageList,
    EFI_HANDLE DriverHandle,
    EFI_HII_HANDLE *Handle
);
typedef EFI_STATUS (EFIAPI *HII_SET_STRING)(
    const HII_STRING_PROTOCOL *This,
    EFI_HII_HANDLE PackageList,
    EFI_STRING_ID StringId,
    const char *Language,
    uint16_t *String,
    const void *StringFontInfo
);
#if NOMADIX_ENABLE_FAN_PROFILES
typedef EFI_STATUS (EFIAPI *HII_EXTRACT_CONFIG)(
    const HII_CONFIG_ACCESS_PROTOCOL *This,
    const uint16_t *Request,
    uint16_t **Progress,
    uint16_t **Results
);
typedef EFI_STATUS (EFIAPI *HII_ROUTE_CONFIG)(
    const HII_CONFIG_ACCESS_PROTOCOL *This,
    const uint16_t *Configuration,
    uint16_t **Progress
);
typedef EFI_STATUS (EFIAPI *HII_FORM_CALLBACK)(
    const HII_CONFIG_ACCESS_PROTOCOL *This,
    UINTN Action,
    uint16_t QuestionId,
    uint8_t Type,
    void *Value,
    UINTN *ActionRequest
);
#endif

struct HII_DATABASE_PROTOCOL {
    HII_NEW_PACKAGE_LIST NewPackageList;
};

struct HII_STRING_PROTOCOL {
    void *NewString;
    void *GetString;
    HII_SET_STRING SetString;
};

#if NOMADIX_ENABLE_FAN_PROFILES
struct HII_CONFIG_ACCESS_PROTOCOL {
    HII_EXTRACT_CONFIG ExtractConfig;
    HII_ROUTE_CONFIG RouteConfig;
    HII_FORM_CALLBACK Callback;
};
#if NOMADIX_PERSIST_FAN_PROFILE
typedef struct {
    EFI_TABLE_HEADER Header;
    void *GetTime;
    void *SetTime;
    void *GetWakeupTime;
    void *SetWakeupTime;
    void *SetVirtualAddressMap;
    void *ConvertPointer;
    EFI_GET_VARIABLE GetVariable;
    void *GetNextVariableName;
    EFI_SET_VARIABLE SetVariable;
} EFI_RUNTIME_SERVICES_VARIABLES;

typedef struct {
    uint32_t Signature;
    uint8_t Version;
    uint8_t Profile;
    uint8_t InverseProfile;
    uint8_t Reserved;
} NOMADIX_FAN_PROFILE_VARIABLE;

typedef char AssertGetVariableOffset[
    offsetof(EFI_RUNTIME_SERVICES_VARIABLES, GetVariable) == 0x48 ? 1 : -1
];
typedef char AssertSetVariableOffset[
    offsetof(EFI_RUNTIME_SERVICES_VARIABLES, SetVariable) == 0x58 ? 1 : -1
];
typedef char AssertFanProfileVariableSize[
    sizeof(NOMADIX_FAN_PROFILE_VARIABLE) == 8U ? 1 : -1
];
#endif
#endif

static const EFI_GUID mHiiDatabaseProtocolGuid = {
    0xEF9FC172,
    0xA1B2,
    0x4693,
    { 0xB3, 0x27, 0x6D, 0x32, 0xFC, 0x41, 0x60, 0x42 }
};
static const EFI_GUID mHiiStringProtocolGuid = {
    0x0FD96974,
    0x23AA,
    0x4CDC,
    { 0xB9, 0xCB, 0x98, 0xD1, 0x77, 0x50, 0x32, 0x2A }
};
static const EFI_GUID mNomadixFormSetGuid = {
    0x5B8D6A31,
    0x74E2,
    0x4F85,
    { 0x9B, 0x1C, 0xA7, 0x36, 0x52, 0x91, 0xD4, 0x0E }
};
static const EFI_GUID mPlatformSetupFormSetGuid = {
    0x93039971,
    0x8545,
    0x4B04,
    { 0xB4, 0x5E, 0x32, 0xEB, 0x83, 0x26, 0x04, 0x0E }
};
#if NOMADIX_ENABLE_FAN_PROFILES
static const EFI_GUID mHiiConfigAccessProtocolGuid = {
    0x330D4706,
    0xF2A0,
    0x4E4F,
    { 0xA3, 0x69, 0xB6, 0x6F, 0xA8, 0xD5, 0x43, 0x85 }
};
#endif

static HII_DATABASE_PROTOCOL *mHiiDatabase;
static HII_STRING_PROTOCOL *mHiiString;
static EFI_HII_HANDLE mNomadixHiiHandle;
static EFI_EVENT mPublishEvent;
static uint8_t mPackageList[HII_PACKAGE_CAPACITY];
static uint8_t mPublished;
static uint8_t mPublishAttemptReported;
#if NOMADIX_ENABLE_FAN_PROFILES
static EFI_HANDLE mHiiDriverHandle;
static uint8_t mConfigAccessInstalled;
static uint16_t mProfileValue[32] = {
    'F', 'u', 'l', 'l', ' ', 'S', 'p', 'e', 'e', 'd', 0
};
#if NOMADIX_PERSIST_FAN_PROFILE
static EFI_RUNTIME_SERVICES_VARIABLES *mRuntimeServices;
static uint8_t mActiveProfile = FAN_PROFILE_FULL;
static uint8_t mPersistedProfile = FAN_PROFILE_FULL;
static uint8_t mPersistedProfileValid;
static const uint16_t mFanProfileVariableName[] = {
    'N', 'o', 'm', 'a', 'd', 'i', 'x', 'F', 'a', 'n', 'P', 'r', 'o',
    'f', 'i', 'l', 'e', 0
};
#endif
#endif

static uint16_t mTempValue[64];
static uint16_t mFan12Value[64];
static uint16_t mFan34Value[64];
static uint16_t mPwmValue[64];

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
    Divisors[0] = (uint8_t)(((Div1 >> 4) & 3U) | ((Vbat >> 3) & 4U));
    Divisors[1] = (uint8_t)(((Div1 >> 6) & 3U) | ((Vbat >> 4) & 4U));
    Divisors[2] = (uint8_t)(((Div2 >> 6) & 3U) | ((Vbat >> 5) & 4U));
    Divisors[3] = (uint8_t)((Diode & 3U) | ((SmiOvt >> 5) & 4U));
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

static void ReadSensorStrings(void)
{
    uint8_t Divisors[4];
    uint32_t Fan[4];
    int32_t Temp1;
    int32_t Temp3;
    UINTN Index;
    UINTN Position;

    Temp1 = (int8_t)W83627Read(0x0027U) * 2;
    Temp3 = (int16_t)W83627ReadWord(0x0250U) / 128;
    ReadFanDivisors(Divisors);
    Fan[0] = FanRpm(W83627Read(0x0028U), Divisors[0]);
    Fan[1] = FanRpm(W83627Read(0x0029U), Divisors[1]);
    Fan[2] = FanRpm(W83627Read(0x002AU), Divisors[2]);
    Fan[3] = FanRpm(W83627Read(0x003FU), Divisors[3]);

    Position = AppendHalfDegrees(mTempValue, 0, Temp1);
    Position = AppendAscii(mTempValue, Position, " C / ");
    Position = AppendHalfDegrees(mTempValue, Position, Temp3);
    AppendAscii(mTempValue, Position, " C");

    Position = AppendUnsigned(mFan12Value, 0, Fan[0]);
    Position = AppendAscii(mFan12Value, Position, " / ");
    Position = AppendUnsigned(mFan12Value, Position, Fan[1]);
    AppendAscii(mFan12Value, Position, " RPM");

    Position = AppendUnsigned(mFan34Value, 0, Fan[2]);
    Position = AppendAscii(mFan34Value, Position, " / ");
    Position = AppendUnsigned(mFan34Value, Position, Fan[3]);
    AppendAscii(mFan34Value, Position, " RPM");

    Position = 0;
    for (Index = 0; Index < 4U; ++Index) {
        uint8_t Pwm;

        Pwm = W83627Read(
            Index == 0U ? 0x0001U :
            Index == 1U ? 0x0003U :
            Index == 2U ? 0x0011U : 0x0061U
        );
        Position = AppendUnsigned(
            mPwmValue,
            Position,
            ((uint32_t)Pwm * 100U + 127U) / 255U
        );
        Position = AppendAscii(
            mPwmValue,
            Position,
#if NOMADIX_ENABLE_FAN_PROFILES
            Index != 3U ? " / " : " %"
#else
            Index != 3U ? "% / " : "%"
#endif
        );
    }
}

#if NOMADIX_ENABLE_FAN_PROFILES
static void W83627Write(uint16_t Register, uint8_t Value)
{
    __outbyte(HWM_INDEX, W83627_BANK_REGISTER);
    __outbyte(HWM_DATA, (uint8_t)(Register >> 8));
    __outbyte(HWM_INDEX, (uint8_t)Register);
    __outbyte(HWM_DATA, Value);
}

static EFI_STATUS ApplyPwmProfile(uint8_t Pwm)
{
    static const uint16_t Registers[4] = {
        0x0001U, 0x0003U, 0x0011U, 0x0061U
    };
    UINTN Index;

    for (Index = 0; Index < 4U; ++Index) {
        W83627Write(Registers[Index], Pwm);
    }
    for (Index = 0; Index < 4U; ++Index) {
        if (W83627Read(Registers[Index]) != Pwm) {
            UINTN RecoveryIndex;

            for (RecoveryIndex = 0; RecoveryIndex < 4U; ++RecoveryIndex) {
                W83627Write(Registers[RecoveryIndex], 0xFFU);
            }
            return EFI_DEVICE_ERROR;
        }
    }
    return EFI_SUCCESS;
}

static void SetProfileName(const char *Name)
{
    UINTN Position;

    Position = 0;
    while (*Name != '\0' && Position + 1U < 32U) {
        mProfileValue[Position++] = (uint8_t)*Name++;
    }
    mProfileValue[Position] = 0;
}

#if NOMADIX_PERSIST_FAN_PROFILE
static const char *FanProfileName(uint8_t Profile)
{
    if (Profile == FAN_PROFILE_QUIET) {
        return "Quiet";
    }
    if (Profile == FAN_PROFILE_BALANCED) {
        return "Balanced";
    }
    return "Full Speed";
}

static uint8_t FanProfilePwm(uint8_t Profile)
{
    if (Profile == FAN_PROFILE_QUIET) {
        return 10U;
    }
    if (Profile == FAN_PROFILE_BALANCED) {
        return 96U;
    }
    return 0xFFU;
}

static EFI_STATUS SaveFanProfile(uint8_t Profile)
{
    EFI_STATUS Status;
    NOMADIX_FAN_PROFILE_VARIABLE Variable;

    if (mRuntimeServices == 0 || mRuntimeServices->SetVariable == 0) {
        return EFI_DEVICE_ERROR;
    }
    if (
        mPersistedProfileValid != 0U &&
        mPersistedProfile == Profile
    ) {
        return EFI_SUCCESS;
    }
    Variable.Signature = FAN_PROFILE_SIGNATURE;
    Variable.Version = FAN_PROFILE_VERSION;
    Variable.Profile = Profile;
    Variable.InverseProfile = (uint8_t)~Profile;
    Variable.Reserved = 0;
    Status = mRuntimeServices->SetVariable(
        mFanProfileVariableName,
        &mNomadixFormSetGuid,
        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
        sizeof(Variable),
        &Variable
    );
    if (Status == EFI_SUCCESS) {
        mPersistedProfile = Profile;
        mPersistedProfileValid = 1U;
    }
    return Status;
}

static EFI_STATUS LoadAndApplyFanProfile(EFI_STATUS *LoadStatus)
{
    EFI_STATUS Status;
    NOMADIX_FAN_PROFILE_VARIABLE Variable;
    UINTN DataSize;
    uint32_t Attributes;
    uint8_t Profile;

    if (
        LoadStatus == 0 ||
        mRuntimeServices == 0 ||
        mRuntimeServices->GetVariable == 0
    ) {
        return EFI_INVALID_PARAMETER;
    }
    DataSize = sizeof(Variable);
    Attributes = 0;
    Status = mRuntimeServices->GetVariable(
        mFanProfileVariableName,
        &mNomadixFormSetGuid,
        &Attributes,
        &DataSize,
        &Variable
    );
    *LoadStatus = Status;
    Profile = FAN_PROFILE_FULL;
    if (Status == EFI_SUCCESS) {
        if (
            DataSize != sizeof(Variable) ||
            Variable.Signature != FAN_PROFILE_SIGNATURE ||
            Variable.Version != FAN_PROFILE_VERSION ||
            Variable.Profile > FAN_PROFILE_FULL ||
            Variable.InverseProfile != (uint8_t)~Variable.Profile ||
            Variable.Reserved != 0U ||
            (
                Attributes &
                (
                    EFI_VARIABLE_NON_VOLATILE |
                    EFI_VARIABLE_BOOTSERVICE_ACCESS
                )
            ) !=
                (
                    EFI_VARIABLE_NON_VOLATILE |
                    EFI_VARIABLE_BOOTSERVICE_ACCESS
                )
        ) {
            *LoadStatus = EFI_COMPROMISED_DATA;
        } else {
            Profile = Variable.Profile;
            mPersistedProfile = Profile;
            mPersistedProfileValid = 1U;
        }
    }
    Status = ApplyPwmProfile(FanProfilePwm(Profile));
    if (Status != EFI_SUCCESS) {
        mActiveProfile = FAN_PROFILE_FULL;
        SetProfileName("Fail-safe Full Speed");
        return Status;
    }
    mActiveProfile = Profile;
    SetProfileName(FanProfileName(Profile));
    return EFI_SUCCESS;
}
#endif

static EFI_STATUS EFIAPI ExtractConfig(
    const HII_CONFIG_ACCESS_PROTOCOL *This,
    const uint16_t *Request,
    uint16_t **Progress,
    uint16_t **Results
)
{
    (void)This;
    (void)Request;
    (void)Progress;
    (void)Results;
    return EFI_UNSUPPORTED;
}

static EFI_STATUS EFIAPI RouteConfig(
    const HII_CONFIG_ACCESS_PROTOCOL *This,
    const uint16_t *Configuration,
    uint16_t **Progress
)
{
    (void)This;
    (void)Configuration;
    (void)Progress;
    return EFI_UNSUPPORTED;
}

static EFI_STATUS EFIAPI ProfileCallback(
    const HII_CONFIG_ACCESS_PROTOCOL *This,
    UINTN Action,
    uint16_t QuestionId,
    uint8_t Type,
    void *Value,
    UINTN *ActionRequest
)
{
    const char *ProfileName;
    EFI_STATUS Status;
    uint8_t Pwm;
#if NOMADIX_PERSIST_FAN_PROFILE
    EFI_STATUS PersistStatus;
    uint8_t Profile;
#endif

    (void)This;
    (void)Type;
    (void)Value;
    if (ActionRequest == 0) {
        return EFI_INVALID_PARAMETER;
    }
    *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
    if (
        Action != EFI_BROWSER_ACTION_CHANGING &&
        Action != EFI_BROWSER_ACTION_CHANGED
    ) {
        return EFI_UNSUPPORTED;
    }
    if (QuestionId == QUESTION_QUIET) {
        Pwm = 10U;
        ProfileName = "Quiet";
#if NOMADIX_PERSIST_FAN_PROFILE
        Profile = FAN_PROFILE_QUIET;
#endif
    } else if (QuestionId == QUESTION_BALANCED) {
        Pwm = 96U;
        ProfileName = "Balanced";
#if NOMADIX_PERSIST_FAN_PROFILE
        Profile = FAN_PROFILE_BALANCED;
#endif
    } else if (QuestionId == QUESTION_FULL) {
        Pwm = 0xFFU;
        ProfileName = "Full Speed";
#if NOMADIX_PERSIST_FAN_PROFILE
        Profile = FAN_PROFILE_FULL;
#endif
    } else {
        return EFI_UNSUPPORTED;
    }
#if NOMADIX_PERSIST_FAN_PROFILE
    if (
        Action == EFI_BROWSER_ACTION_CHANGED &&
        mActiveProfile == Profile &&
        mPersistedProfileValid != 0U &&
        mPersistedProfile == Profile
    ) {
        return EFI_SUCCESS;
    }
#endif
    Status = ApplyPwmProfile(Pwm);
    if (Status != EFI_SUCCESS) {
        SetProfileName("Fail-safe Full Speed");
        return Status;
    }
#if NOMADIX_PERSIST_FAN_PROFILE
    mActiveProfile = Profile;
    PersistStatus = SaveFanProfile(Profile);
    SerialWriteString("[NOMADIX-4GA] NVRAM-SAVE=");
    SerialWriteHex64(PersistStatus);
    SerialWriteString(" PROFILE=");
    SerialWriteHex64(Profile);
    SerialWriteString("\r\n");
#endif
    SetProfileName(ProfileName);
    Status = EFI_SUCCESS;
    if (mPublished != 0U) {
        ReadSensorStrings();
        Status = mHiiString->SetString(
            mHiiString,
            mNomadixHiiHandle,
            HII_STRING_PROFILE_VALUE,
            "en-US",
            mProfileValue,
            0
        );
        if (Status == EFI_SUCCESS) {
            Status = mHiiString->SetString(
                mHiiString,
                mNomadixHiiHandle,
                HII_STRING_PWM_VALUE,
                "en-US",
                mPwmValue,
                0
            );
        }
    }
#if NOMADIX_PERSIST_FAN_PROFILE
    if (PersistStatus != EFI_SUCCESS) {
        return PersistStatus;
    }
#endif
    return Status;
}

static HII_CONFIG_ACCESS_PROTOCOL mConfigAccess = {
    ExtractConfig,
    RouteConfig,
    ProfileCallback
};
#endif

static void Write8(UINTN *Position, uint8_t Value)
{
    mPackageList[(*Position)++] = Value;
}

static void Write16(UINTN *Position, uint16_t Value)
{
    Write8(Position, (uint8_t)Value);
    Write8(Position, (uint8_t)(Value >> 8));
}

static void Write32(UINTN *Position, uint32_t Value)
{
    Write16(Position, (uint16_t)Value);
    Write16(Position, (uint16_t)(Value >> 16));
}

static void Patch32(UINTN Position, uint32_t Value)
{
    mPackageList[Position] = (uint8_t)Value;
    mPackageList[Position + 1U] = (uint8_t)(Value >> 8);
    mPackageList[Position + 2U] = (uint8_t)(Value >> 16);
    mPackageList[Position + 3U] = (uint8_t)(Value >> 24);
}

static void WriteGuid(UINTN *Position, const EFI_GUID *Guid)
{
    UINTN Index;

    Write32(Position, Guid->Data1);
    Write16(Position, Guid->Data2);
    Write16(Position, Guid->Data3);
    for (Index = 0; Index < 8U; ++Index) {
        Write8(Position, Guid->Data4[Index]);
    }
}

static void WriteText(
    UINTN *Position,
    uint16_t Prompt,
    uint16_t Help,
    uint16_t Value
)
{
    Write8(Position, 0x03U);
    Write8(Position, 0x08U);
    Write16(Position, Prompt);
    Write16(Position, Help);
    Write16(Position, Value);
}

#if NOMADIX_ENABLE_FAN_PROFILES
static void WriteAction(
    UINTN *Position,
    uint16_t Prompt,
    uint16_t Help,
    uint16_t QuestionId
)
{
    Write8(Position, 0x0CU);
    Write8(Position, 0x8FU);
    Write16(Position, Prompt);
    Write16(Position, Help);
    Write16(Position, QuestionId);
    Write16(Position, 0);
    Write16(Position, 0xFFFFU);
    Write8(Position, 0x04U);
    Write16(Position, 0);
    Write8(Position, 0x29U);
    Write8(Position, 0x02U);
}
#endif

static void WriteUcs2String(UINTN *Position, const uint16_t *String)
{
    Write8(Position, 0x14U);
    do {
        Write16(Position, *String);
    } while (*String++ != 0U);
}

static void WriteAsciiString(UINTN *Position, const char *String)
{
    Write8(Position, 0x14U);
    do {
        Write16(Position, (uint8_t)*String);
    } while (*String++ != '\0');
}

static EFI_STATUS BuildPackageList(void)
{
    UINTN FormsHeader;
    UINTN FormsStart;
    UINTN Index;
    UINTN Position;
    UINTN StringsHeader;
    UINTN StringsStart;
    static const char *const StaticStrings[] = {
        "English",
        "Nomadix Hardware Monitor",
        "Read-only Winbond W83627DHG-P sensor status",
        "Super I/O",
        "Winbond W83627DHG-P",
        "Hardware monitor base",
        "0x0290",
        "Board temperatures",
        0,
        "Fan 1 / Fan 2",
        0,
        "Fan 3 / Fan 4",
        0,
        "PWM 1 / 2 / 3 / 4",
        0
    };

    ReadSensorStrings();
    Position = 0;
    WriteGuid(&Position, &mNomadixFormSetGuid);
    Write32(&Position, 0);

    FormsHeader = Position;
    Write32(&Position, 0);
    FormsStart = Position;
    Write8(&Position, 0x0EU);
    Write8(&Position, 0xA7U);
    WriteGuid(&Position, &mNomadixFormSetGuid);
    Write16(&Position, HII_STRING_FORM_TITLE);
    Write16(&Position, HII_STRING_FORM_HELP);
    Write8(&Position, 1U);
    WriteGuid(&Position, &mPlatformSetupFormSetGuid);

    Write8(&Position, 0x01U);
    Write8(&Position, 0x86U);
    Write16(&Position, HII_FORM_ID);
    Write16(&Position, HII_STRING_FORM_TITLE);
    WriteText(
        &Position,
        HII_STRING_CHIP_PROMPT,
        HII_STRING_FORM_HELP,
        HII_STRING_CHIP_VALUE
    );
    WriteText(
        &Position,
        HII_STRING_BASE_PROMPT,
        HII_STRING_FORM_HELP,
        HII_STRING_BASE_VALUE
    );
    WriteText(
        &Position,
        HII_STRING_TEMP_PROMPT,
        HII_STRING_FORM_HELP,
        HII_STRING_TEMP_VALUE
    );
    WriteText(
        &Position,
        HII_STRING_FAN12_PROMPT,
        HII_STRING_FORM_HELP,
        HII_STRING_FAN12_VALUE
    );
    WriteText(
        &Position,
        HII_STRING_FAN34_PROMPT,
        HII_STRING_FORM_HELP,
        HII_STRING_FAN34_VALUE
    );
    WriteText(
        &Position,
        HII_STRING_PWM_PROMPT,
        HII_STRING_FORM_HELP,
        HII_STRING_PWM_VALUE
    );
#if NOMADIX_ENABLE_FAN_PROFILES
    WriteText(
        &Position,
        HII_STRING_PROFILE_PROMPT,
        HII_STRING_FORM_HELP,
        HII_STRING_PROFILE_VALUE
    );
    WriteAction(
        &Position,
        HII_STRING_QUIET_ACTION,
        HII_STRING_QUIET_HELP,
        QUESTION_QUIET
    );
    WriteAction(
        &Position,
        HII_STRING_BALANCED_ACTION,
        HII_STRING_BALANCED_HELP,
        QUESTION_BALANCED
    );
    WriteAction(
        &Position,
        HII_STRING_FULL_ACTION,
        HII_STRING_FULL_HELP,
        QUESTION_FULL
    );
#endif
    Write8(&Position, 0x29U);
    Write8(&Position, 0x02U);
    Write8(&Position, 0x29U);
    Write8(&Position, 0x02U);
    Patch32(
        FormsHeader,
        (uint32_t)(Position - FormsHeader) | 0x02000000U
    );

    StringsHeader = Position;
    Write32(&Position, 0);
    Write32(&Position, 52U);
    Write32(&Position, 52U);
    for (Index = 0; Index < 16U; ++Index) {
        Write16(&Position, 0);
    }
    Write16(&Position, HII_STRING_LANGUAGE_NAME);
    Write8(&Position, 'e');
    Write8(&Position, 'n');
    Write8(&Position, '-');
    Write8(&Position, 'U');
    Write8(&Position, 'S');
    Write8(&Position, 0);
    StringsStart = Position;
    for (Index = 0; Index < 15U; ++Index) {
        if (StaticStrings[Index] != 0) {
            WriteAsciiString(&Position, StaticStrings[Index]);
        } else if (Index == 8U) {
            WriteUcs2String(&Position, mTempValue);
        } else if (Index == 10U) {
            WriteUcs2String(&Position, mFan12Value);
        } else if (Index == 12U) {
            WriteUcs2String(&Position, mFan34Value);
        } else {
            WriteUcs2String(&Position, mPwmValue);
        }
    }
#if NOMADIX_ENABLE_FAN_PROFILES
    WriteAsciiString(&Position, "Active fan profile");
    WriteUcs2String(&Position, mProfileValue);
    WriteAsciiString(&Position, "Apply Quiet profile");
    WriteAsciiString(&Position, "Set all four PWM channels to 10/255.");
    WriteAsciiString(&Position, "Apply Balanced profile");
    WriteAsciiString(&Position, "Set all four PWM channels to 96/255.");
    WriteAsciiString(&Position, "Apply Full Speed profile");
    WriteAsciiString(&Position, "Set all four PWM channels to 255/255.");
#endif
    Write8(&Position, 0);
    Patch32(
        StringsHeader,
        (uint32_t)(Position - StringsHeader) | 0x04000000U
    );
    if (StringsStart - StringsHeader != 52U) {
        return EFI_DEVICE_ERROR;
    }
    Write32(&Position, 0xDF000004U);
    Patch32(16U, (uint32_t)Position);
    return Position <= sizeof(mPackageList) ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}

static EFI_STATUS UpdatePublishedStrings(void)
{
    EFI_STATUS Status;

    ReadSensorStrings();
    Status = mHiiString->SetString(
        mHiiString,
        mNomadixHiiHandle,
        HII_STRING_TEMP_VALUE,
        "en-US",
        mTempValue,
        0
    );
    if (Status == EFI_SUCCESS) {
        Status = mHiiString->SetString(
            mHiiString,
            mNomadixHiiHandle,
            HII_STRING_FAN12_VALUE,
            "en-US",
            mFan12Value,
            0
        );
    }
    if (Status == EFI_SUCCESS) {
        Status = mHiiString->SetString(
            mHiiString,
            mNomadixHiiHandle,
            HII_STRING_FAN34_VALUE,
            "en-US",
            mFan34Value,
            0
        );
    }
    if (Status == EFI_SUCCESS) {
        Status = mHiiString->SetString(
            mHiiString,
            mNomadixHiiHandle,
            HII_STRING_PWM_VALUE,
            "en-US",
            mPwmValue,
            0
        );
    }
#if NOMADIX_ENABLE_FAN_PROFILES
    if (Status == EFI_SUCCESS) {
        Status = mHiiString->SetString(
            mHiiString,
            mNomadixHiiHandle,
            HII_STRING_PROFILE_VALUE,
            "en-US",
            mProfileValue,
            0
        );
    }
#endif
    return Status;
}

static void EFIAPI PublishOrRefreshForm(EFI_EVENT Event, void *Context)
{
    EFI_LOCATE_PROTOCOL LocateProtocol;
    EFI_STATUS Status;

    (void)Event;
    (void)Context;
    if (mPublished != 0U) {
        UpdatePublishedStrings();
        return;
    }
    LocateProtocol = GetLocateProtocol();
    Status = LocateProtocol(
        &mHiiDatabaseProtocolGuid,
        0,
        (void **)&mHiiDatabase
    );
    if (Status != EFI_SUCCESS) {
        return;
    }
    Status = LocateProtocol(
        &mHiiStringProtocolGuid,
        0,
        (void **)&mHiiString
    );
    if (Status != EFI_SUCCESS) {
        return;
    }
#if NOMADIX_ENABLE_FAN_PROFILES
    if (mConfigAccessInstalled == 0U) {
        EFI_BOOT_SERVICES_WITH_CRC *BootServices;

        BootServices = (EFI_BOOT_SERVICES_WITH_CRC *)mBootServices;
        Status = BootServices->InstallMultipleProtocolInterfaces(
            &mHiiDriverHandle,
            &mHiiConfigAccessProtocolGuid,
            &mConfigAccess,
            0
        );
        if (Status != EFI_SUCCESS) {
            return;
        }
        mConfigAccessInstalled = 1U;
    }
#endif
    Status = BuildPackageList();
    if (Status != EFI_SUCCESS) {
        return;
    }
    Status = mHiiDatabase->NewPackageList(
        mHiiDatabase,
        mPackageList,
#if NOMADIX_ENABLE_FAN_PROFILES
        mHiiDriverHandle,
#else
        0,
#endif
        &mNomadixHiiHandle
    );
    if (mPublishAttemptReported == 0U) {
        SerialWriteString("\r\n[NOMADIX-4FY] PUBLISH=");
        SerialWriteHex64(Status);
        SerialWriteString(" HANDLE=");
        SerialWriteHex64((uint64_t)(uintptr_t)mNomadixHiiHandle);
        SerialWriteString("\r\n");
        mPublishAttemptReported = 1U;
    }
    if (Status != EFI_SUCCESS) {
        return;
    }
    mPublished = 1U;
    GetSetTimer()(mPublishEvent, TIMER_PERIODIC, 10000000U);
}

EFI_STATUS EFIAPI NOMADIX_STAGE4FY_ENTRY(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;
#if NOMADIX_PERSIST_FAN_PROFILE
    EFI_STATUS ApplyStatus;
    EFI_STATUS LoadStatus;
#endif
    static const uint16_t ArmedMessage[] = {
        '\r', '\n',
        '[', 'N', 'O', 'M', 'A', 'D', 'I', 'X', '-', '4',
        NOMADIX_STAGE4FY_MARKER_GROUP,
        NOMADIX_STAGE4FY_MARKER_STAGE, ']',
        ' ', 'H', 'I', 'I', '-', 'F', 'O', 'R', 'M', 'S', 'E', 'T',
        '-', 'A', 'R', 'M', 'E', 'D',
        '\r', '\n',
        0
    };

    Status = Stage4FSEfiMain(ImageHandle, SystemTable);
    if (Status != EFI_SUCCESS) {
        return Status;
    }
#if NOMADIX_PERSIST_FAN_PROFILE
    mRuntimeServices =
        (EFI_RUNTIME_SERVICES_VARIABLES *)SystemTable->RuntimeServices;
    LoadStatus = EFI_NOT_READY;
    ApplyStatus = LoadAndApplyFanProfile(&LoadStatus);
    SerialWriteString("[NOMADIX-4GA] NVRAM-LOAD=");
    SerialWriteHex64(LoadStatus);
    SerialWriteString(" APPLY=");
    SerialWriteHex64(ApplyStatus);
    SerialWriteString(" PROFILE=");
    SerialWriteHex64(mActiveProfile);
    SerialWriteString("\r\n");
#endif
    mPublishEvent = 0;
    Status = mBootServices->CreateEvent(
        EVT_TIMER | EVT_NOTIFY_SIGNAL,
        TPL_CALLBACK,
        PublishOrRefreshForm,
        0,
        &mPublishEvent
    );
    if (Status == EFI_SUCCESS) {
        Status = GetSetTimer()(mPublishEvent, TIMER_PERIODIC, 100000U);
    }
    if (Status != EFI_SUCCESS) {
        return Status;
    }
    PublishOrRefreshForm(mPublishEvent, 0);
    return NOMADIX_STAGE_MESSAGE(ArmedMessage);
}
