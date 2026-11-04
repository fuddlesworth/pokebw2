#ifndef POKEBW2_FIELD_COLOSSEUM_TOOL_H
#define POKEBW2_FIELD_COLOSSEUM_TOOL_H

#include "types.h"
#include "field/union_comm.h"
#include "struct_decls.h"

// colosseum_tool.c, overlay 34 (the name is a guess, the ROM has no string of it): the colosseum's checks of who is
// here and ready, and the spots that its players stand on

// Whether all the players are here, and whether the cards of all of them are in
BOOL UnionEff_AllColosseumMembersHere(UnionColosseum *colosseum);
BOOL UnionEff_AllColosseumCardsIn(UnionColosseum *colosseum);
BOOL UnionEff_AreColosseumPositionsReceived(UnionColosseum *colosseum);
void UnionEff_ClearColosseumPositionsReceived(UnionColosseum *colosseum);
BOOL UnionEff_GetEntrySpot(Field *field, u32 kind, u32 *index);
BOOL UnionEff_AreAllColosseumMembersLeaving(UnionColosseum *colosseum);
BOOL UnionEff_IsAnyColosseumMemberLeaving(UnionColosseum *colosseum);
BOOL UnionEff_IsOnSeatSpot(Field *field);
void UnionEff_UpdateColosseumCommPlayers(UnionColosseum *colosseum);

#endif // POKEBW2_FIELD_COLOSSEUM_TOOL_H
