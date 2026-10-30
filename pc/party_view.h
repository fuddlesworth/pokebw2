#ifndef POKEBW2_PC_PARTY_VIEW_H
#define POKEBW2_PC_PARTY_VIEW_H

#include "pml/poke_party.h"
#include "ui_draw.h"
#include "ui_layout.h"

typedef struct {
    PartyPkm *pkm;
    u32 hp, maxHp, level, status, type1, type2, ability, item;
    u32 attack, defense, speed, spAttack, spDefense;
    u32 moves[4], pp[4], maxPp[4];
    bool inBattle;
} PcPartyMember;

typedef struct {
    Font *font;
    PcPartyMember members[6];
    u32 count, selected, marked;
    bool overview, backView;
    bool interactiveRows, interactiveTabs;
    PcUiActionKind rowAction;
    const char *section;
} PcPartyView;

void PcPartyView_ReadMember(PcPartyMember *member, PartyPkm *pkm);
bool PcPartyView_Draw(const PcPartyView *view);
void PcPartyView_DrawDetails(const PcPartyView *view);
void PcPartyView_DrawMoves(const PcPartyView *view);
void PcPartyView_DrawHeldItem(const PcPartyView *view);

#endif
