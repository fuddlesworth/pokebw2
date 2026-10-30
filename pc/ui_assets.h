#ifndef POKEBW2_PC_UI_ASSETS_H
#define POKEBW2_PC_UI_ASSETS_H

#include "types.h"
#include "struct_decls.h"

void PcUi_ItemIcon(u32 item, u32 slot, float x, float y, float size);
void PcUi_RibbonIcon(u32 chars, u32 palette, u32 slot, float x, float y, float size);
void PcUi_PokemonIcon(PartyPkm *pkm, u32 slot, float x, float y, float size);
void PcUi_BattleGender(u32 sex, float x, float y, float height);

void PcUi_TypeBadge(u32 type, u32 slot, u32 language, float x, float y, float width, float height);
void PcUi_CenteredTypeBadge(u32 type, u32 slot, u32 language, float x, float y, float width, float height);

#endif
