#include "nitro/wm.h"

WMErrCode WM_SetGameInfo(WMCallbackFunc callback, const u16 *userGameInfo, u16 gameInfoSize, u32 ggid, u16 tgid,
                         u8 attr) {
    // The native port has no Nintendo wireless manager to publish local-radio beacons through.
    return WM_ERRCODE_ILLEGAL_STATE;
}
