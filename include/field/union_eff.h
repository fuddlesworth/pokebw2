#ifndef POKEBW2_FIELD_UNION_EFF_H
#define POKEBW2_FIELD_UNION_EFF_H

#include "types.h"
#include "field/colosseum_tool.h"
#include "field/field_actor.h"
#include "field/union_comm.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// union_eff.c, overlay 34, which the ROM names: the Union Room's effects on the field, the focus on a player and
// the marks over them (the field keeps its work). The colosseum's checks of who is ready are colosseum_tool.c's,
// whose header this one includes

// Whether the player stands on the Union Room's spot (the one entry of its table)
BOOL UnionEff_IsPlayerOnSpot(Field *field);
// The UNION_WANT_* of a UNION_ACTIVITY_*, which marks the players who want the same: the battles, the drawing, the
// trade, the spin trade and 0x1a
int UnionEff_GetKindStep(int activity);
// Whether the beacon's player is one of the wifi list's friends
BOOL UnionEff_IsFriendBeacon(WifiList *wifiList, const UnionBeacon *beacon);
UnionEffect *UnionEff_Create(HeapID heapId);
void UnionEff_Free(UnionEffect *work);
void UnionEff_Update(UnionEffect *work);
void UnionEff_Draw(UnionEffect *work);
// Focus on an actor
void UnionEff_AddFocus(UnionEffect *work, FieldActor *actor);
// Put a mark over an actor, the played one when played is TRUE and else the friend one
void UnionEff_AddMark(UnionEffect *work, FieldActor *actor, BOOL played);
// Take the mark off an actor
void UnionEff_RemoveMark(UnionEffect *work, FieldActor *actor);

#endif // POKEBW2_FIELD_UNION_EFF_H
