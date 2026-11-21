#ifndef POKEBW2_APP_MUSICAL_STA_ACT_BUTTON_H
#define POKEBW2_APP_MUSICAL_STA_ACT_BUTTON_H

// Overlay 209's sta_act_button.c: the touch screen's two buttons during a musical, one for each prop the player's
// Pokémon holds in its hands, which use the prop when touched

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "musical/musical.h"
#include "struct_decls.h"

// What StaActButton.selected holds when no button was touched
#define STA_ACT_BUTTON_NONE 2

struct StaActButton {
    HeapID heapId;
    StaActing *stage;
    MusicalPoke *poke;
    // Whether the buttons can be touched
    BOOL active;
    // Whether a prop was touched and its use not yet started
    BOOL pressed;
    u32 selected;
    ClActUnit *clactUnit;
    // The button's, the prop's and the touch effect's resources
    u32 palettes[3];
    u32 chars[3];
    u32 cellAnims[3];
    // Whether each hand's prop was used
    BOOL used[2];
    u16 itemIds[2];
    ClActor *buttons[2];
    ClActor *items[2];
    ClActor *effect;
};

StaActButton *StaActButton_InitSystem(HeapID heapId, StaActing *stage, MusicalPoke *poke);
void StaActButton_TermSystem(StaActButton *sys);
void StaActButton_UpdateSystem(StaActButton *sys);
void StaActButton_SetShowFlg(StaActButton *sys, BOOL show);

#endif // POKEBW2_APP_MUSICAL_STA_ACT_BUTTON_H
