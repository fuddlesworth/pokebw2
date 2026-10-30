#include "save.h"
#include "dwc/bm.h"
#include "dwc/init.h"
#include "gfl/backup_system.h"
#include "nitro/host_card.h"
#include "save/join_avenue.h"
#include "save/save_control.h"
#include "settings.h"
#include "system/text_speed.h"
#include <SDL3/SDL.h>

static bool initialized;

u32 DWCi_GetGameCode(void) {
    return 0x4952414a; // The shared online game code used by DWCRapCommon_Init.
}
void DWC_Auth_GetId(DWCAuthWiFiId *id) {
    // No DS firmware Wi-Fi identity is provisioned on a new PC installation.
    SDL_memset(id, 0, sizeof(*id));
}

bool PcSave_Init(void) {
    if (initialized)
        return true;
    const char *override = SDL_getenv("POKEBW2_SAVE_FILE");
    char *path = NULL;
    if (!override) {
        char *directory = SDL_GetPrefPath("pokebw2", "pokebw2-pc");
        if (!directory)
            return false;
        int length = SDL_asprintf(&path, "%sblack2.sav", directory);
        SDL_free(directory);
        if (length < 0)
            return false;
    }
    bool opened = PcCard_OpenBackup(override ? override : path);
    SDL_free(path);
    if (!opened)
        return false;
    PcSettings_Load();
    mainBackupSystemInit(HEAPID_USER, NULL, NULL);
    saveMainSaveAreaAddress(SaveControl_Create(HEAPID_USER));
    setImportantJABlockAddresses(SaveControl_GetInstance());
    initialized = true;
    return true;
}
void PcSave_Shutdown(void) {
    if (initialized)
        SaveControl_Free();
    PcCard_CloseBackup();
    initialized = false;
}
