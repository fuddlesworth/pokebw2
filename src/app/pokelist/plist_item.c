#include "app/pokelist.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "pml/evolution.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "system/bmp_winframe.h"
#include "system/game_beacon.h"
#include "system/game_comm.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/party_list.h"

// Using an item from the bag on a Pokémon of the party list, and the stats shown after a level up. The ROM doesn't
// name this file; it is named for what it does

// The item effect follows the named fields of ItemParams; the party list treats battle-only boosts as none
// The cure flags packed from ItemParams
#define POKELIST_CURE_SLEEP 0x1
#define POKELIST_CURE_POISON 0x2
#define POKELIST_CURE_BURN 0x4
#define POKELIST_CURE_FREEZE 0x8
#define POKELIST_CURE_PARALYSIS 0x10
#define POKELIST_CURE_CONFUSION 0x20
#define POKELIST_CURE_ALL 0x3f

static u32 PokeList_GetItemEffect(u16 item);
static BOOL PokeList_HasEffort(PartyPkm *pkm, u32 param, u16 item);
static void PokeList_ShowItemMessage(PokeListWork *wk, u32 offset, void (*doneFunc)(PokeListWork *wk));
static void PokeList_ItemHealDone(PokeListWork *wk);
static void PokeList_ItemReviveDone(PokeListWork *wk);
static void PokeList_ItemReviveNext(PokeListWork *wk);
static void PokeList_ShowPkmMessage(PokeListWork *wk, u32 msgId);
static void PokeList_ShowStatMessage(PokeListWork *wk, u32 msgId, u16 stat);
static void PokeList_LevelUpMessageDone(PokeListWork *wk);

// The stats in the order the level up shows them
static const u16 sStatParams[] = {
    PKM_PARAM_MAX_HP, PKM_PARAM_ATTACK, PKM_PARAM_DEFENSE, PKM_PARAM_SP_ATTACK, PKM_PARAM_SP_DEFENSE, PKM_PARAM_SPEED,
};

static u32 PokeList_GetItemEffect(u16 item) {
    void *data = PML_ItemReadDataFile(item, ITEM_FILE_DATA, HEAPID_POKELIST);
    s32 cures;
    s32 effort;

    if (PML_ItemGetParam(data, ITEM_PARAM_WORK_TYPE) != ITEM_WORK_PARAMS) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_UNUSABLE;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_ATTACK_STAGES) != 0 ||
        PML_ItemGetParam(data, ITEM_PARAM_DEFENSE_STAGES) != 0 ||
        PML_ItemGetParam(data, ITEM_PARAM_SP_ATTACK_STAGES) != 0 ||
        PML_ItemGetParam(data, ITEM_PARAM_SP_DEFENSE_STAGES) != 0 ||
        PML_ItemGetParam(data, ITEM_PARAM_SPEED_STAGES) != 0 ||
        PML_ItemGetParam(data, ITEM_PARAM_ACCURACY_STAGES) != 0 ||
        PML_ItemGetParam(data, ITEM_PARAM_CRIT_STAGES) != 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_NONE;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_REVIVE_ALL) != 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_REVIVE_ALL;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_LEVEL_UP) != 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_LEVEL_UP;
    }
    cures = PML_ItemGetParam(data, ITEM_PARAM_SLEEP_HEAL);
    cures += PML_ItemGetParam(data, ITEM_PARAM_POISON_HEAL) << 1;
    cures += PML_ItemGetParam(data, ITEM_PARAM_BURN_HEAL) << 2;
    cures += PML_ItemGetParam(data, ITEM_PARAM_FREEZE_HEAL) << 3;
    cures += PML_ItemGetParam(data, ITEM_PARAM_PARALYSIS_HEAL) << 4;
    cures += PML_ItemGetParam(data, ITEM_PARAM_CONFUSION_HEAL) << 5;
    switch (cures) {
    case POKELIST_CURE_SLEEP:
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_CURE_SLEEP;
    case POKELIST_CURE_POISON:
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_CURE_POISON;
    case POKELIST_CURE_BURN:
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_CURE_BURN;
    case POKELIST_CURE_FREEZE:
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_CURE_FREEZE;
    case POKELIST_CURE_PARALYSIS:
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_CURE_PARALYSIS;
    case POKELIST_CURE_CONFUSION:
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_CURE_CONFUSION;
    case POKELIST_CURE_ALL:
        if (PML_ItemGetParam(data, ITEM_PARAM_HP_RESTORE) != 0) {
            GFL_HeapFree(data);
            return POKELIST_ITEM_EFFECT_RESTORE_HP;
        }
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_CURE_ALL_STATUS;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_INFATUATION_HEAL) != 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_CURE_INFATUATION;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_HP_RESTORE) != 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_RESTORE_HP;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_REVIVE) != 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_REVIVE;
    }
    effort = PML_ItemGetParam(data, ITEM_PARAM_HP_EV);
    if (effort > 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_RAISE_HP_EV;
    }
    if (effort < 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_LOWER_HP_EV;
    }
    effort = PML_ItemGetParam(data, ITEM_PARAM_ATTACK_EV);
    if (effort > 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_RAISE_ATTACK_EV;
    }
    if (effort < 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_LOWER_ATTACK_EV;
    }
    effort = PML_ItemGetParam(data, ITEM_PARAM_DEFENSE_EV);
    if (effort > 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_RAISE_DEFENSE_EV;
    }
    if (effort < 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_LOWER_DEFENSE_EV;
    }
    effort = PML_ItemGetParam(data, ITEM_PARAM_SPEED_EV);
    if (effort > 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_RAISE_SPEED_EV;
    }
    if (effort < 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_LOWER_SPEED_EV;
    }
    effort = PML_ItemGetParam(data, ITEM_PARAM_SP_ATTACK_EV);
    if (effort > 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_RAISE_SP_ATTACK_EV;
    }
    if (effort < 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_LOWER_SP_ATTACK_EV;
    }
    effort = PML_ItemGetParam(data, ITEM_PARAM_SP_DEFENSE_EV);
    if (effort > 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_RAISE_SP_DEFENSE_EV;
    }
    if (effort < 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_LOWER_SP_DEFENSE_EV;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_EVOLVE) != 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_EVOLVE;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_PP_UP) != 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_PP_UP;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_PP_MAX) != 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_PP_MAX;
    }
    if (PML_ItemGetParam(data, ITEM_PARAM_PP_RESTORE) != 0 || PML_ItemGetParam(data, ITEM_PARAM_PP_RESTORE_ALL) != 0) {
        GFL_HeapFree(data);
        return POKELIST_ITEM_EFFECT_RESTORE_PP;
    }
    GFL_HeapFree(data);
    return POKELIST_ITEM_EFFECT_UNUSABLE;
}

// Whether the item is used on one of the Pokémon's moves, which the menu picks
BOOL PokeList_IsItemForMove(PokeListWork *wk, u16 item) {
    switch (PokeList_GetItemEffect(item)) {
    case POKELIST_ITEM_EFFECT_PP_UP:
        return TRUE;
    case POKELIST_ITEM_EFFECT_PP_MAX:
        return TRUE;
    case POKELIST_ITEM_EFFECT_RESTORE_PP:
        if (GetItemParam(item, ITEM_PARAM_PP_RESTORE, wk->heapId)) {
            return TRUE;
        }
    }
    return FALSE;
}

// Whether the item is used on the whole party at once, as Sacred Ash is
BOOL PokeList_IsItemForParty(PokeListWork *wk, u16 item) {
    if (GetItemParam(item, ITEM_PARAM_REVIVE_ALL, wk->heapId)) {
        return TRUE;
    }
    return FALSE;
}

// The first Pokémon of the party that the item can be used on, or -1
s32 PokeList_FindItemTarget(PokeListWork *wk) {
    u8 count = PokeParty_GetPkmCount(wk->param->party);
    u8 i;

    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(wk->param->party, i);

        if (StatusRcv_CanUseItem(pkm, wk->param->item, 0, wk->heapId) == TRUE) {
            return i;
        }
    }
    return -1;
}

// The message of the menu that picks a move for the item
u32 PokeList_GetItemMenuMessage(PokeListWork *wk, u16 item) {
    switch (PokeList_GetItemEffect(item)) {
    case POKELIST_ITEM_EFFECT_PP_UP:
    case POKELIST_ITEM_EFFECT_PP_MAX:
        return PartyList_Text_BoostPpWhichMove;
    case POKELIST_ITEM_EFFECT_RESTORE_PP:
        return PartyList_Text_RestoreWhichMove;
    }
    return PartyList_Text_RestoreWhichMove;
}

// Changes Arceus's type to the plate it holds
void PokeList_UpdateArceusForm(PokeListWork *wk, PartyPkm *pkm, u16 item) {
    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) == SPECIES_ARCEUS) {
        u32 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        u16 type = _getTypeForPlate(item);

        if (form != type) {
            PokeParty_ChangeForme(pkm, type);
        }
    }
}

// Changes Genesect's form to the drive it holds
void PokeList_UpdateGenesectForm(PokeListWork *wk, PartyPkm *pkm, u16 item) {
    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) == SPECIES_GENESECT) {
        u32 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        u32 driveForm = PML_ItemGetGenesectDriveForme(item);

        if (form != driveForm) {
            PokeParty_ChangeForme(pkm, driveForm);
        }
    }
}

// Shows message 0x52 + offset with the item's name
static void PokeList_ShowItemMessage(PokeListWork *wk, u32 offset, void (*doneFunc)(PokeListWork *wk)) {
    wk->param->result = POKELIST_RESULT_ITEM_DONE;
    PokeListMessage_CreateWordSet(wk, wk->message);
    PokeListMessage_SetItemName(wk, wk->message, 0, wk->param->item);
    PokeList_ShowMessage(wk, offset + PartyList_Text_WontHaveAnyEffect, TRUE, doneFunc);
    PokeListMessage_FreeWordSet(wk, wk->message);
}

// The item had no effect: the list ends
void PokeList_ShowItemUselessExit(PokeListWork *wk) {
    PokeList_ShowItemMessage(wk, POKELIST_ITEM_MESSAGE_NO_EFFECT, PokeList_MessageDoneExit);
}

void PokeList_ShowItemUseless(PokeListWork *wk) {
    PokeList_ShowItemMessage(wk, POKELIST_ITEM_MESSAGE_NO_EFFECT, PokeList_MessageDoneSelect);
}

void PokeList_ShowItemMessageSelect(PokeListWork *wk, u32 offset) {
    PokeList_ShowItemMessage(wk, offset, PokeList_MessageDoneSelect);
}

// Shows what the item did, and returns its effect
u32 PokeList_ShowItemResult(PokeListWork *wk, u32 move) {
    u32 effect = PokeList_GetItemEffect(wk->param->item);

    switch (effect) {
    case POKELIST_ITEM_EFFECT_REVIVE_ALL:
        wk->state = POKELIST_STATE_HP_CHANGE_START;
        wk->prevHp = PokeListPlate_GetHp(wk, wk->plates[wk->cursorPos]);
        wk->hpDoneFunc = PokeList_ItemReviveDone;
        break;
    case POKELIST_ITEM_EFFECT_LEVEL_UP: {
        u32 level = PokeParty_GetLevel(wk->pkm);
        StrBuf *name;
        u8 i;

        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
        PokeListMessage_SetNumber(wk, wk->message, 1, level + 1, 3);
        PokeList_ShowMessage(wk, PartyList_Text_ElevatedLv, TRUE, PokeList_LevelUpMessageDone);
        PokeListMessage_FreeWordSet(wk, wk->message);
        name = GFL_StrBufCreate(12, wk->heapId);
        PokeParty_GetParam(wk->pkm, PKM_PARAM_NICKNAME, name);
        GameBeaconSys_SendNickname(name);
        GFL_StrBufFree(name);
        for (i = 0; i < 6; i++) {
            wk->prevStats[i] = PokeParty_GetParam(wk->pkm, sStatParams[i], NULL);
        }
        break;
    }
    case POKELIST_ITEM_EFFECT_CURE_SLEEP:
        PokeList_ShowPkmMessage(wk, PartyList_Text_WokeUp);
        break;
    case POKELIST_ITEM_EFFECT_CURE_POISON:
        PokeList_ShowPkmMessage(wk, PartyList_Text_CuredItsPoisoning);
        break;
    case POKELIST_ITEM_EFFECT_CURE_BURN:
        PokeList_ShowPkmMessage(wk, PartyList_Text_SBurnHealed);
        break;
    case POKELIST_ITEM_EFFECT_CURE_FREEZE:
        PokeList_ShowPkmMessage(wk, PartyList_Text_ThawedOut);
        break;
    case POKELIST_ITEM_EFFECT_CURE_PARALYSIS:
        PokeList_ShowPkmMessage(wk, PartyList_Text_CuredParalysis);
        break;
    case POKELIST_ITEM_EFFECT_CURE_ALL_STATUS:
        PokeList_ShowPkmMessage(wk, PartyList_Text_BecameHealthy);
        break;
    case POKELIST_ITEM_EFFECT_RESTORE_HP:
    case POKELIST_ITEM_EFFECT_REVIVE:
        wk->state = POKELIST_STATE_HP_CHANGE_START;
        wk->prevHp = PokeListPlate_GetHp(wk, wk->plates[wk->cursorPos]);
        wk->hpDoneFunc = PokeList_ItemHealDone;
        break;
    case POKELIST_ITEM_EFFECT_RAISE_HP_EV:
    case POKELIST_ITEM_EFFECT_RAISE_ATTACK_EV:
    case POKELIST_ITEM_EFFECT_RAISE_DEFENSE_EV:
    case POKELIST_ITEM_EFFECT_RAISE_SPEED_EV:
    case POKELIST_ITEM_EFFECT_RAISE_SP_ATTACK_EV:
    case POKELIST_ITEM_EFFECT_RAISE_SP_DEFENSE_EV:
        PokeList_ShowStatMessage(wk, PartyList_Text_SBaseRose, effect - POKELIST_ITEM_EFFECT_RAISE_HP_EV);
        break;
    case POKELIST_ITEM_EFFECT_LOWER_HP_EV:
    case POKELIST_ITEM_EFFECT_LOWER_ATTACK_EV:
    case POKELIST_ITEM_EFFECT_LOWER_DEFENSE_EV:
    case POKELIST_ITEM_EFFECT_LOWER_SPEED_EV:
    case POKELIST_ITEM_EFFECT_LOWER_SP_ATTACK_EV:
    case POKELIST_ITEM_EFFECT_LOWER_SP_DEFENSE_EV: {
        u16 stat = effect - POKELIST_ITEM_EFFECT_LOWER_HP_EV;

        if (PokeList_HasEffort(wk->pkm, PKM_PARAM_EV_HP + stat, wk->param->item) == FALSE) {
            PokeList_ShowStatMessage(wk, PartyList_Text_BecameMoreFriendlyIts, stat);
        } else if (PokeParty_GetParam(wk->pkm, PKM_PARAM_HAPPINESS, NULL) == 255) {
            PokeList_ShowStatMessage(wk, PartyList_Text_AdoresItsBaseFell, stat);
        } else {
            PokeList_ShowStatMessage(wk, PartyList_Text_BecameMoreFriendlyHowever, stat);
        }
        break;
    }
    case POKELIST_ITEM_EFFECT_PP_UP:
    case POKELIST_ITEM_EFFECT_PP_MAX:
        PokeListMessage_CreateWordSet(wk, wk->message);
        PokeListMessage_SetMoveName(wk, wk->message, 0, move);
        PokeList_ShowMessage(wk, PartyList_Text_SPpIncreased, TRUE, PokeList_MessageDoneItem);
        PokeListMessage_FreeWordSet(wk, wk->message);
        break;
    case POKELIST_ITEM_EFFECT_RESTORE_PP:
        PokeList_ShowPkmMessage(wk, PartyList_Text_PpRestored);
        break;
    case POKELIST_ITEM_EFFECT_UNUSABLE:
        break;
    }
    return effect;
}

static void PokeList_ItemHealDone(PokeListWork *wk) {
    u16 hp = PokeListPlate_GetHp(wk, wk->plates[wk->cursorPos]);

    wk->param->result = POKELIST_RESULT_ITEM_DONE;
    PokeListMessage_CreateWordSet(wk, wk->message);
    PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
    if (wk->prevHp != 0) {
        PokeListMessage_SetNumber(wk, wk->message, 1, hp - wk->prevHp, 3);
        PokeList_ShowMessage(wk, PartyList_Text_SHpRestoredBy, TRUE, PokeList_MessageDoneItem);
    } else {
        PokeList_ShowMessage(wk, PartyList_Text_RecoveredFromFainting, TRUE, PokeList_MessageDoneItem);
    }
    PokeListMessage_FreeWordSet(wk, wk->message);
}

static void PokeList_ItemReviveDone(PokeListWork *wk) {
    wk->param->result = POKELIST_RESULT_ITEM_DONE;
    PokeListMessage_CreateWordSet(wk, wk->message);
    PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
    if (PokeList_FindItemTarget(wk) != -1) {
        PokeList_ShowMessage(wk, PartyList_Text_RecoveredFromFainting, TRUE, PokeList_ItemReviveNext);
    } else {
        PokeList_ShowMessage(wk, PartyList_Text_RecoveredFromFainting, TRUE, PokeList_MessageDoneExit);
    }
    PokeListMessage_FreeWordSet(wk, wk->message);
}

// Revives the next fainted Pokémon, for an item used on the whole party
static void PokeList_ItemReviveNext(PokeListWork *wk) {
    s32 pos = PokeList_FindItemTarget(wk);

    PokeListMessage_Close(wk, wk->message);
    PokeListPlate_SetPalette(wk, wk->plates[wk->cursorPos], POKELIST_PLATE_PALETTE_NORMAL);
    wk->cursorPos = pos;
    wk->pkm = PokeParty_GetPkm(wk->param->party, pos);
    PokeList_ShowItemResult(wk, 0);
    StatusRcv_UseItem(wk->pkm, wk->param->item, 0, wk->param->zoneId, wk->heapId);
    PokeListPlate_Redraw(wk, wk->plates[wk->cursorPos]);
    GFL_SndSEPlay(SEQ_SE_RECOVERY);
}

static void PokeList_LevelUpMessageDone(PokeListWork *wk) {
    wk->state = POKELIST_STATE_LEVEL_UP;
    wk->subState = POKELIST_LEVELUP_DRAW_GAINS;
    wk->param->learnIndex = 0;
    PokeListMessage_DrawKeyCursor(wk, wk->message);
}

// Called once the message of a move learned is read: on to the next move
void PokeList_LearnMessageDone(PokeListWork *wk) {
    wk->state = POKELIST_STATE_LEVEL_UP;
    wk->subState = POKELIST_LEVELUP_LEARN_MOVES;
}

// The level up: shows the stats' gains, then the stats, then teaches the moves learned at the level, and checks for
// an evolution
void PokeList_UpdateLevelUp(PokeListWork *wk) {
    switch (wk->subState) {
    case POKELIST_LEVELUP_DRAW_GAINS: {
        BOOL bigGain = FALSE;
        u8 i;
        u8 row;

        wk->statsWindow = BmpWin_CreateDynamic(0, 1, 1, 14, 12, 14, TRUE);
        BmpWin_DrawFrame(wk->statsWindow, WINFRAME_TRANSFER_VBLANK, 1, 12);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->statsWindow), 15);
        BmpWin_Transfer(wk->statsWindow);
        for (i = 0; i < 6; i++) {
            u32 gain = PokeParty_GetParam(wk->pkm, sStatParams[i], NULL) - wk->prevStats[i];
            StrBuf *name;
            StrBuf *format;
            StrBuf *str;
            WordSet *wordSet;
            u16 width;

            if (gain >= 10) {
                bigGain = TRUE;
            }
            name = GFL_MsgDataLoadStrbufNew(wk->msgData, PartyList_Text_MaxHp + i);
            PrintQueue_PrintColor(wk->printQueue, BmpWin_GetBitmap(wk->statsWindow), 0, i * 16, name, wk->font, 0x440);
            GFL_StrBufFree(name);
            format = GFL_MsgDataLoadStrbufNew(wk->msgData, PartyList_Text_Empty_52);
            str = GFL_StrBufCreate(32, wk->heapId);
            wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
            WordSetNumber(wordSet, 0, gain, 3, 0, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, format);
            width = GFL_FontGetBlockWidth(str, wk->font, 0);
            PrintQueue_PrintColor(wk->printQueue, BmpWin_GetBitmap(wk->statsWindow), 104 - width, i * 16, str, wk->font,
                                  0x440);
            GFL_StrBufFree(format);
            GFL_StrBufFree(str);
            GFL_WordSetSystemFree(wordSet);
        }
        for (row = 0; row < 6; row++) {
            u8 x = bigGain == TRUE ? 72 : 80;
            StrBuf *plus = GFL_MsgDataLoadStrbufNew(wk->msgData, PartyList_Text_Empty_51);

            PrintQueue_PrintColor(wk->printQueue, BmpWin_GetBitmap(wk->statsWindow), x, row * 16, plus, wk->font,
                                  0x440);
            GFL_StrBufFree(plus);
        }
        wk->subState = POKELIST_LEVELUP_TRANSFER_GAINS;
        break;
    }
    case POKELIST_LEVELUP_TRANSFER_GAINS:
        if (PrintQueue_IsBitmapPending(wk->printQueue, BmpWin_GetBitmap(wk->statsWindow)) == FALSE) {
            BmpWin_Transfer(wk->statsWindow);
            wk->subState = POKELIST_LEVELUP_WAIT_GAINS;
        }
        break;
    case POKELIST_LEVELUP_WAIT_GAINS:
        if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) ||
            GFL_HIDIsTouchPressed() == TRUE) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            wk->subState = POKELIST_LEVELUP_DRAW_STATS;
        }
        break;
    case POKELIST_LEVELUP_DRAW_STATS: {
        u8 i;

        GFL_BitmapFillArea(BmpWin_GetBitmap(wk->statsWindow), 80, 0, 32, 96, 15);
        for (i = 0; i < 6; i++) {
            u32 stat = PokeParty_GetParam(wk->pkm, sStatParams[i], NULL);
            StrBuf *format = GFL_MsgDataLoadStrbufNew(wk->msgData, PartyList_Text_Empty_52);
            StrBuf *str = GFL_StrBufCreate(32, wk->heapId);
            WordSet *wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
            u16 width;

            WordSetNumber(wordSet, 0, stat, 3, 0, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, str, format);
            width = GFL_FontGetBlockWidth(str, wk->font, 0);
            PrintQueue_PrintColor(wk->printQueue, BmpWin_GetBitmap(wk->statsWindow), 104 - width, i * 16, str, wk->font,
                                  0x440);
            GFL_StrBufFree(format);
            GFL_StrBufFree(str);
            GFL_WordSetSystemFree(wordSet);
        }
        wk->subState = POKELIST_LEVELUP_TRANSFER_STATS;
        break;
    }
    case POKELIST_LEVELUP_TRANSFER_STATS:
        if (PrintQueue_IsBitmapPending(wk->printQueue, BmpWin_GetBitmap(wk->statsWindow)) == FALSE) {
            BmpWin_Transfer(wk->statsWindow);
            wk->subState = POKELIST_LEVELUP_WAIT_STATS;
        }
        break;
    case POKELIST_LEVELUP_WAIT_STATS:
        if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) ||
            GFL_HIDIsTouchPressed() == TRUE) {
            wk->subState = POKELIST_LEVELUP_LEARN_MOVES;
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            BmpWin_ClearScreen(wk->statsWindow);
            BmpWin_ClearFrame(wk->statsWindow, WINFRAME_TRANSFER_VBLANK);
            BmpWin_Free(wk->statsWindow);
            wk->statsWindow = NULL;
        }
        break;
    case POKELIST_LEVELUP_LEARN_MOVES: {
        u16 move = PokeParty_LearnNextLevelMove(wk->pkm, &wk->param->learnIndex, wk->heapId);

        PokeListMessage_Close(wk, wk->message);
        if (move == 0) {
            wk->subState = POKELIST_LEVELUP_CHECK_EVOLUTION;
        } else if (move == LEARN_MOVE_KNOWN) {
            wk->subState = POKELIST_LEVELUP_CHECK_EVOLUTION;
        } else if (move & LEARN_MOVE_NO_SLOT) {
            wk->param->move = move - LEARN_MOVE_NO_SLOT;
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetMoveName(wk, wk->message, 1, wk->param->move);
            PokeList_ShowMessage(wk, PartyList_Text_WantsLearnMoveHowever, FALSE, PokeList_AskReplaceMove);
            PokeListMessage_FreeWordSet(wk, wk->message);
        } else {
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
            PokeListMessage_SetMoveName(wk, wk->message, 1, move);
            PokeList_ShowMessage(wk, PartyList_Text_Learned, TRUE, PokeList_LearnMessageDone);
            PokeListMessage_FreeWordSet(wk, wk->message);
        }
        break;
    }
    case POKELIST_LEVELUP_CHECK_EVOLUTION: {
        // Read but not used
        u32 species = PokeParty_GetParam(wk->pkm, PKM_PARAM_SPECIES, NULL);

        if (CheckEvolveSpecies(wk->param->party, wk->pkm, EVO_CHECK_LEVEL_UP, wk->param->zoneId, wk->param->season,
                               NULL, wk->heapId) != 0) {
            wk->state = POKELIST_STATE_EXIT_WAIT_BUTTON;
            wk->param->index = wk->cursorPos;
            wk->param->result = POKELIST_RESULT_EVOLVE_LEVEL;
        } else {
            wk->param->mode = POKELIST_MODE_USE_ITEM;
            wk->param->index = wk->cursorPos;
            wk->param->result = POKELIST_RESULT_ITEM_DONE;
            PokeList_MessageDoneItem(wk);
        }
        break;
    }
    }
}

static BOOL PokeList_HasEffort(PartyPkm *pkm, u32 param, u16 item) {
    BOOL result = FALSE;

    if (PokeParty_GetParam(pkm, param, NULL) != 0) {
        result = TRUE;
    }
    return result;
}

static void PokeList_ShowPkmMessage(PokeListWork *wk, u32 msgId) {
    wk->param->result = POKELIST_RESULT_ITEM_DONE;
    PokeListMessage_CreateWordSet(wk, wk->message);
    PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
    PokeList_ShowMessage(wk, msgId, TRUE, PokeList_MessageDoneItem);
    PokeListMessage_FreeWordSet(wk, wk->message);
}

static void PokeList_ShowStatMessage(PokeListWork *wk, u32 msgId, u16 stat) {
    wk->param->result = POKELIST_RESULT_ITEM_DONE;
    PokeListMessage_CreateWordSet(wk, wk->message);
    PokeListMessage_SetPkmName(wk, wk->message, 0, wk->pkm);
    PokeListMessage_SetStatName(wk, wk->message, 1, stat);
    PokeList_ShowMessage(wk, msgId, TRUE, PokeList_MessageDoneItem);
    PokeListMessage_FreeWordSet(wk, wk->message);
}
