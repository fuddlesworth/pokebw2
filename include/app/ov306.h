#ifndef POKEBW2_APP_OV306_H
#define POKEBW2_APP_OV306_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Overlay 306's screen, which the Battle Subway and the battle proc show

#define OVERLAY_OV306 OVERLAY_ID(306)

// The fields have the same roles as BtlRecSelParam in app/btl_rec_sel.h. Existing types are retained.
typedef struct {
    GameData *gameData;
    u32 compatible;
    u32 comm;
    u32 recorded;
    u32 mode;
    s32 counter;
} Ov306Param;

extern const GameProcFunctions BtlRecSelProcFunctions;

#endif // POKEBW2_APP_OV306_H
