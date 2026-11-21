#ifndef POKEBW2_APP_WIN_RECORD_H
#define POKEBW2_APP_WIN_RECORD_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"

// The Pokémon World Tournament's win record (win_record.c), of overlay 326, which runs with a WbtOv326Param of
// field/wbt.h. The names are ours
#define OVERLAY_WIN_RECORD OVERLAY_ID(326)

extern const GameProcFunctions WIN_RECORD_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WIN_RECORD_H
