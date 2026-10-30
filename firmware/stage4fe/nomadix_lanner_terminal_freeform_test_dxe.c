#define NOMADIX_STAGE4FC_ENTRY EfiMain
#define NOMADIX_STAGE_PREFIX "[NOMADIX-4FE]"
#define NOMADIX_LANNER_FROM_FV
#define NOMADIX_LANNER_LOAD_ONLY
#define NOMADIX_TERMINAL_FILE_GUID_INITIALIZER \
    0xD3C9A4E1, \
    0x7B62, \
    0x4A41, \
    {0x91, 0xE8, 0x54, 0x42, 0x10, 0x73, 0xFC, 0x2D}
#include "../stage4fc/nomadix_lanner_terminal_test_dxe.c"
