#ifndef POKEBW2_APP_MUSICAL_STA_ACT_AUDIENCE_H
#define POKEBW2_APP_MUSICAL_STA_ACT_AUDIENCE_H

// Overlay 209's sta_act_audience.c: the audience on the touch screen, 60 spectators drawn in its BGs, who turn to
// look at the Pokémon they cheer for or at a light

#include "types.h"
#include "gfl/heap.h"
#include "musical/musical_stage_sys.h"
#include "struct_decls.h"

#define STA_ACT_AUDIENCE_COUNT 60

// What a spectator looks at when no Pokémon or light is picked
#define STA_ACT_AUDIENCE_NONE 0xff

typedef struct {
    // The position of the Pokémon looked at, or STA_ACT_AUDIENCE_NONE
    u8 target;
    // Frames until the spectator turns again
    u8 timer;
    // Which Pokémon's fan it is, which picks its look
    u8 type;
    // Set to STA_ACT_AUDIENCE_NONE once the member is hidden
    u16 hidden;
    // Where it sits, in 4 by 4 tile blocks
    u8 x;
    u8 y;
} StaActAudienceMember;

struct StaActAudience {
    HeapID heapId;
    u8 unk2[0x1e6];
    StaActing *stage;
    // Whether each Pokémon is being cheered for
    BOOL cheer[4];
    // The light everyone looks at, or STA_ACT_AUDIENCE_NONE
    u8 lookTarget;
    // Whether the spectators pick what to look at again
    BOOL refresh;
    StaActAudienceMember members[STA_ACT_AUDIENCE_COUNT];
};

StaActAudience *StaActAudience_InitSystem(HeapID heapId, StaActing *stage, MusicalStageParam *param);
void StaActAudience_TermSystem(StaActAudience *sys);
void StaActAudience_UpdateSystem(StaActAudience *sys);
void StaActAudience_SetCheerPoke(StaActAudience *sys, u8 pos, BOOL cheer);
void StaActAudience_SetLookLight(StaActAudience *sys, u8 light);
void StaActAudience_SetScrollOffset(StaActAudience *sys, int scroll);

#endif // POKEBW2_APP_MUSICAL_STA_ACT_AUDIENCE_H
