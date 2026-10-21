// TwlSDK's DSi part of os_ownerInfoEx.c (ARM9 main), in the LTD autoload: the wireless settings that main's
// OS_IsAvailableWireless and OS_IsForceDisableWireless read on a DSi. The file name is inferred from main's half, as
// card_rom_twl.c's is; the functions' names are ours
#include "nitro/os.h"
#include "twl/os.h"

// The DSi's secure hardware settings that the system leaves in the shared memory, of which only the flag read here is
// named. The flag is a bit of a byte of flags, after the word of valid languages (as ntrtwl's reconstruction lays
// out TwlSDK's OSTWLHWSecureInfo), which the load reaches as offset 4 of the block
typedef struct TWLHwSecureInfo {
    u32 unk_00;
    struct {
        u8 forceDisableWireless : 1;
        u8 : 7;
    } flags;
    u8 unk_05[3];
} TWLHwSecureInfo;

#define HW_TWL_HW_SECURE_INFO 0x02fffd68

#pragma define_section LTDMAIN ".ltdmain"                                                                              \
                               ".ltdmain.bss"                                                                          \
                               ".ltdmain" abs32 RWX
#pragma section LTDMAIN begin

BOOL OSi_IsAvailableWirelessTwl(void) {
    return TWL_SETTINGS->wirelessEnabled == 1 ? TRUE : FALSE;
}

BOOL OSi_IsForceDisableWirelessTwl(void) {
    TWLHwSecureInfo *info = (TWLHwSecureInfo *)HW_TWL_HW_SECURE_INFO;

    return info->flags.forceDisableWireless == 1 ? TRUE : FALSE;
}

#pragma section LTDMAIN end
