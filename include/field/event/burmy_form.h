#ifndef POKEBW2_FIELD_EVENT_BURMY_FORM_H
#define POKEBW2_FIELD_EVENT_BURMY_FORM_H

// Overlay 12's Burmy form change, a file of its own before event_battle.c. Function name from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"

// Changes Burmy's form for the terrain it was caught on
void burmyTransform(GameData *gameData, PartyPkm *pkm, u32 terrain);

#endif // POKEBW2_FIELD_BURMY_FORM_H
