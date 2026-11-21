#ifndef POKEBW2_FIELD_MAP_SYMBOL_SAVE_FIELD_H
#define POKEBW2_FIELD_MAP_SYMBOL_SAVE_FIELD_H

// Overlay 12's symbol_save_field.c (a descriptive name): what the field reads of the symbol encounters of the Entree
// Forest, which the save keeps in its area NPC data, encrypted

#include "types.h"
#include "entralink/entree_forest.h"
#include "struct_decls.h"
#include "system/savedata/symbol_save.h"

u32 SymbolSaveField_GetGroup(AreaNPCSave *npcData);
u32 SymbolSaveField_GetDeepestArea(AreaNPCSave *npcData);
// The Pokémon of an area, and how many there are
u32 SymbolSaveField_GetAreaPokemon(AreaNPCSave *npcData, EntreeForestPokemon *dest, int max, int area, u8 *count);

#endif // POKEBW2_FIELD_SYMBOL_SAVE_FIELD_H
