#ifndef POKEBW2_APP_T_DOWNLOAD_H
#define POKEBW2_APP_T_DOWNLOAD_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"

// The Pokémon World Tournament's downloaded tournaments (t_download.c), of overlay 326: the tournaments received over
// Wi-Fi, the ones kept in the save, and their details. It runs with a WbtOv326Param2 of field/wbt.h. The names are ours
#define OVERLAY_T_DOWNLOAD OVERLAY_ID(326)

extern const GameProcFunctions T_DOWNLOAD_PROC_FUNCTIONS;

#endif // POKEBW2_APP_T_DOWNLOAD_H
