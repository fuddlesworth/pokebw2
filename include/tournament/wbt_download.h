#ifndef POKEBW2_APP_WBT_DOWNLOAD_H
#define POKEBW2_APP_WBT_DOWNLOAD_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Downloads battle tournaments over Wi-Fi
#define OVERLAY_WBT_DOWNLOAD OVERLAY_ID(327)

typedef struct {
    GameSystem *gsys;
    GameData *gameData;
} WbtDownloadParam;

extern const GameProcFunctions WBT_DOWNLOAD_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WBT_DOWNLOAD_H
