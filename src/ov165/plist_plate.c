#include "app/pokelist.h"
#include "battle/regulation.h"
#include "constants/arc.h"
#include "constants/battle.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "field/musical.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "system/hp_gauge.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/btl_main_11.h"

// A Pokémon's plate in the party list: its name, level, HP, icon, ball, item and status

// Each actor resource has its own animation sequences
enum {
    STATUS_ICON_ANM_PARALYSIS = 1,
    STATUS_ICON_ANM_FREEZE = 2,
    STATUS_ICON_ANM_SLEEP = 3,
    STATUS_ICON_ANM_POISON = 4,
    STATUS_ICON_ANM_BURN = 5,
    STATUS_ICON_ANM_FAINTED = 6,
};

enum {
    PARTY_BALL_ANM_NORMAL = 0,
    PARTY_BALL_ANM_SELECTED = 1,
};

struct PokeListPlate {
    PartyPkm *pkm;
    u8 index;
    // The HP shown, which moves toward targetHp
    u16 hp;
    u16 targetHp;
    BOOL isEgg;
    BOOL selected;
    BOOL isEmpty;
    // Whether the window waits for its text to be printed, to be copied to the BG
    BOOL nameDirty;
    // The Pokémon's place in the battle's order from 0, POKELIST_ENTRY_* otherwise
    int entry;
    BmpWin *window;
    ClActRenderer *renderer;
    ClActUnit *unit;
    u32 iconChars;
    ClActor *icon;
    ClActor *ball;
    ClActor *hpBar;
    ClActor *itemIcon;
    ClActor *statusIcon;
    // Frames into the icon's hop
    u8 bounceTimer;
};

static PokeListPlate *PokeListPlate_CreateCore(PokeListWork *wk, u8 index, PartyPkm *pkm);
static void PokeListPlate_CreateActors(PokeListWork *wk, PokeListPlate *plate);
static void PokeListPlate_CreateIcon(PokeListWork *wk, PokeListPlate *plate);
static void PokeListPlate_Draw(PokeListWork *wk, PokeListPlate *plate, BOOL numbersOnly);
static void PokeListPlate_DrawInfo(PokeListWork *wk, PokeListPlate *plate, BOOL numbersOnly);
static void PokeListPlate_DrawHpBar(PokeListWork *wk, PokeListPlate *plate);
static void PokeListPlate_DrawEgg(PokeListWork *wk, PokeListPlate *plate);
static void PokeListPlate_RedrawNumbers(PokeListWork *wk, PokeListPlate *plate);
static void PokeListPlate_UpdateEntry(PokeListWork *wk, PokeListPlate *plate);
static u8 PokeListPlate_GetHpColor(PokeListPlate *plate);
static void PokeListPlate_GetIconPos(PokeListPlate *plate, int x, int y, ClActorPos *pos);

// The tile that each plate's top left corner is at
static const u8 sPlatePos[POKELIST_PLATE_COUNT][2] = {
    { 0, 1 }, { 16, 2 }, { 0, 7 }, { 16, 8 }, { 0, 13 }, { 16, 14 },
};

// Where the plate's screen is in the plates' screen file: the first plate's, then the others'
static const u8 sPlateScreenPos[2][2] = {
    { 0, 1 },
    { 16, 2 },
};

static PokeListPlate *PokeListPlate_CreateCore(PokeListWork *wk, u8 index, PartyPkm *pkm) {
    u8 i;
    u8 x = sPlatePos[index][0];
    u8 y = sPlatePos[index][1];
    int windowX;
    BOOL isEmpty;
    PokeListPlate *plate;
    u8 screenX;
    u8 screenY;
    ClActSurfaceSetup surfaceSetup;
    ClActorPos pos;

    plate = GFL_HeapAllocate(wk->heapId, sizeof(PokeListPlate), FALSE, "plist_plate.c", 175);
    plate->index = index;
    plate->pkm = pkm;
    isEmpty = FALSE;
    if (pkm == NULL) {
        isEmpty = TRUE;
    }
    plate->isEmpty = isEmpty;
    plate->selected = FALSE;
    plate->nameDirty = FALSE;
    if (pkm != NULL) {
        screenX = sPlateScreenPos[index != 0 ? 1 : 0][0];
        screenY = sPlateScreenPos[index != 0 ? 1 : 0][1];
    } else {
        screenX = 0;
        screenY = 19;
    }
    windowX = x + 16;
    GFL_BGSysLoadScrArea(2, windowX, y, 16, 6, wk->plateScreen->rawData, screenX, screenY, 32, 32);
    surfaceSetup.x = 0;
    surfaceSetup.y = 0;
    surfaceSetup.w = 256;
    surfaceSetup.h = 192;
    surfaceSetup.screen = 0;
    surfaceSetup.culling = 0;
    plate->renderer = ClActRenderer_Create(&surfaceSetup, 1, wk->heapId);
    plate->unit = ClActUnit_Create(6, 0, wk->heapId);
    ClActUnit_SetRenderer(plate->unit, plate->renderer);
    PokeListPlate_CreateActors(wk, plate);
    PokeListPlate_CreateIcon(wk, plate);
    pos.x = -x * 8;
    pos.y = -y * 8;
    ClActRenderer_SetSurfacePos(plate->renderer, 0, &pos);
    plate->window = BmpWin_CreateDynamic(1, windowX, y, 16, 6, 13, TRUE);
    if (pkm != NULL) {
        plate->isEgg = PokeParty_GetParam(plate->pkm, PKM_PARAM_IS_EGG, NULL);
        plate->hp = PokeParty_GetParam(plate->pkm, PKM_PARAM_HP, NULL);
        plate->targetHp = plate->hp;
        ClActor_SetVisible(plate->hpBar, TRUE);
        PokeListPlate_UpdateEntry(wk, plate);
        if (PokeList_IsBattle(wk) == TRUE) {
            for (i = 0; i < 6; i++) {
                if (plate->index + 1 == wk->param->picked[i]) {
                    plate->entry = i;
                }
            }
        }
        PokeListPlate_SetSelected(wk, plate, FALSE);
        PokeListPlate_Draw(wk, plate, FALSE);
    } else {
        plate->entry = POKELIST_ENTRY_EMPTY;
        GFL_BGSysQueueScrLoad(2);
    }
    return plate;
}

PokeListPlate *PokeListPlate_Create(PokeListWork *wk, u8 index, PartyPkm *pkm) {
    return PokeListPlate_CreateCore(wk, index, pkm);
}

PokeListPlate *PokeListPlate_CreateEmpty(PokeListWork *wk, u8 index) {
    return PokeListPlate_CreateCore(wk, index, NULL);
}

void PokeListPlate_Free(PokeListWork *wk, PokeListPlate *plate) {
    BmpWin_Free(plate->window);
    ClActor_Delete(plate->icon);
    ClActor_Delete(plate->ball);
    ClActor_Delete(plate->hpBar);
    ClActRenderer_Delete(plate->renderer);
    ClActUnit_Delete(plate->unit);
    ClActSys_FreeChars(plate->iconChars);
    GFL_HeapFree(plate);
}

BOOL PokeListPlate_IsPrinting(PokeListPlate *plate) {
    return plate->nameDirty;
}

void PokeListPlate_Update(PokeListWork *wk, PokeListPlate *plate) {
    if (plate->isEmpty == FALSE) {
        if (plate->nameDirty == TRUE &&
            PrintQueue_IsBitmapPending(wk->printQueue, BmpWin_GetBitmap(plate->window)) == FALSE) {
            plate->nameDirty = FALSE;
            BmpWin_Transfer(plate->window);
        }
        if (plate->selected == TRUE && GetStatusCond(plate->pkm) == CONDITION_NONE && plate->hp != 0) {
            ClActorPos pos;

            PokeListPlate_GetIconPos(plate, 26, 19, &pos);
            if (plate->bounceTimer >= 10) {
                pos.y -= 6;
                if (plate->bounceTimer >= 20) {
                    plate->bounceTimer = 0;
                }
            }
            plate->bounceTimer++;
            ClActor_SetSurfacePos(plate->icon, &pos, 0);
        }
    }
}

static void PokeListPlate_CreateActors(PokeListWork *wk, PokeListPlate *plate) {
    ClActorSetup ballSetup;
    ClActorSetup hpBarSetup;
    ClActorSetup itemSetup;
    ClActorSetup statusSetup;

    ballSetup.x = 16;
    ballSetup.y = 16;
    ballSetup.sequence = PARTY_BALL_ANM_NORMAL;
    ballSetup.priority = 1;
    ballSetup.bgPriority = 2;
    plate->ball = ClActor_Create(plate->unit, wk->clResources[CL_RES_CHAR(1)], wk->clResources[CL_RES_PLTT(0)],
                                 wk->clResources[CL_RES_CELL(1)], &ballSetup, 0, wk->heapId);
    ClActor_SetAutoAnim(plate->ball, TRUE);
    hpBarSetup.x = 84;
    hpBarSetup.y = 28;
    hpBarSetup.sequence = 0;
    hpBarSetup.priority = 0;
    hpBarSetup.bgPriority = 2;
    plate->hpBar = ClActor_Create(plate->unit, wk->clResources[CL_RES_CHAR(6)], wk->clResources[CL_RES_PLTT(5)],
                                  wk->clResources[CL_RES_CELL(6)], &hpBarSetup, 0, wk->heapId);
    ClActor_SetVisible(plate->hpBar, FALSE);
    itemSetup.x = 32;
    itemSetup.y = 24;
    itemSetup.sequence = POKELIST_HELD_ITEM_ANM_ITEM;
    itemSetup.priority = 0;
    itemSetup.bgPriority = 2;
    plate->itemIcon = ClActor_Create(plate->unit, wk->clResources[CL_RES_CHAR(3)], wk->clResources[CL_RES_PLTT(2)],
                                     wk->clResources[CL_RES_CELL(3)], &itemSetup, 0, wk->heapId);
    ClActor_SetVisible(plate->itemIcon, FALSE);
    statusSetup.x = 32;
    statusSetup.y = 42;
    statusSetup.sequence = 0;
    statusSetup.priority = 0;
    statusSetup.bgPriority = 2;
    plate->statusIcon = ClActor_Create(plate->unit, wk->clResources[CL_RES_CHAR(5)], wk->clResources[CL_RES_PLTT(4)],
                                       wk->clResources[CL_RES_CELL(5)], &statusSetup, 0, wk->heapId);
    ClActor_SetVisible(plate->statusIcon, FALSE);
}

static void PokeListPlate_CreateIcon(PokeListWork *wk, PokeListPlate *plate) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_TAIL(wk->heapId));
    u32 file;
    u8 palette;
    ClActorSetup setup;

    if (plate->pkm != NULL) {
        BoxPkm *pkm = PokeParty_ToBoxPkm(plate->pkm);

        file = PokeIcon_GetFileOfPkm(pkm);
        palette = PokeIcon_GetPaletteOfPkm(pkm);
    } else {
        file = PokeParty_GetIconIndex(1, 0, 0, TRUE);
        palette = PokeIcon_GetPalette(1, 0, 0, TRUE);
    }
    plate->iconChars = ClActSys_LoadChars(arc, file, FALSE, 0, wk->heapId);
    GFL_ArcToolFree(arc);
    setup.x = 24;
    setup.y = 16;
    setup.sequence = POKELIST_PARTY_ICON_ANM_DEFAULT;
    setup.priority = 0;
    setup.bgPriority = 2;
    plate->icon = ClActor_Create(plate->unit, plate->iconChars, wk->clResources[CL_RES_PLTT(6)],
                                 wk->clResources[CL_RES_CELL(7)], &setup, 0, wk->heapId);
    ClActor_SetPalette(plate->icon, palette, 1);
    ClActor_SetAutoAnim(plate->icon, TRUE);
    ClActor_SetVisible(plate->icon, plate->pkm != NULL ? TRUE : FALSE);
}

static void PokeListPlate_Draw(PokeListWork *wk, PokeListPlate *plate, BOOL numbersOnly) {
    if (plate->isEgg == FALSE) {
        PokeListPlate_DrawInfo(wk, plate, numbersOnly);
        PokeListPlate_DrawHpBar(wk, plate);
    } else {
        PokeListPlate_DrawEgg(wk, plate);
    }
}

// Prints the name, sex and level, and the HP or what the Pokémon can do in the mode; numbersOnly draws the numbers
// at once, as the HP changes
static void PokeListPlate_DrawInfo(PokeListWork *wk, PokeListPlate *plate, BOOL numbersOnly) {
    BOOL showLearn = FALSE;
    u32 item;

    if (wk->param->mode == POKELIST_MODE_TEACH_TM || wk->wasTutorMode == TRUE) {
        showLearn = TRUE;
    }
    if (numbersOnly == FALSE) {
        WordSet *wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);

        loadPokemonNicknameToStrbuf(wordSet, 0, plate->pkm);
        PokeList_PrintWordSetString(wk, plate->window, wordSet, BtlMain11_Text_Empty, 41, 9, 0x440);
        GFL_WordSetSystemFree(wordSet);
    }
    if (numbersOnly == FALSE && PokeParty_GetParam(plate->pkm, PKM_PARAM_SHOW_SEX, NULL) == TRUE) {
        u32 sex = PokeParty_GetParam(plate->pkm, PKM_PARAM_SEX, NULL);

        if (sex == GENDER_MALE) {
            PokeList_PrintString(wk, plate->window, BtlMain11_Text_Empty_5, 105, 9, 0x14c0);
        } else if (sex == GENDER_FEMALE) {
            PokeList_PrintString(wk, plate->window, BtlMain11_Text_Empty_6, 105, 9, 0xc80);
        }
    }
    if (showLearn == FALSE) {
        if ((GetStatusCond(plate->pkm) == CONDITION_NONE && plate->hp != 0) || plate->entry != POKELIST_ENTRY_EMPTY) {
            WordSet *wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);

            WordSetNumber(wordSet, 0, PokeParty_GetLevel(plate->pkm), 3, 0, TRUE);
            if (numbersOnly == FALSE) {
                PokeList_PrintWordSetStringSmall(wk, plate->window, wordSet, BtlMain11_Text_L, 9, 35, 0x440);
            } else {
                PokeList_DrawWordSetStringSmall(wk, plate->window, wordSet, BtlMain11_Text_L, 9, 35, 0x440);
            }
            GFL_WordSetSystemFree(wordSet);
            ClActor_SetVisible(plate->statusIcon, FALSE);
        } else {
            if (plate->hp == 0) {
                ClActor_SetSequence(plate->statusIcon, STATUS_ICON_ANM_FAINTED);
            } else {
                switch (GetStatusCond(plate->pkm)) {
                case CONDITION_PARALYSIS:
                    ClActor_SetSequence(plate->statusIcon, STATUS_ICON_ANM_PARALYSIS);
                    break;
                case CONDITION_SLEEP:
                    ClActor_SetSequence(plate->statusIcon, STATUS_ICON_ANM_SLEEP);
                    break;
                case CONDITION_FREEZE:
                    ClActor_SetSequence(plate->statusIcon, STATUS_ICON_ANM_FREEZE);
                    break;
                case CONDITION_BURN:
                    ClActor_SetSequence(plate->statusIcon, STATUS_ICON_ANM_BURN);
                    break;
                case CONDITION_POISON:
                    ClActor_SetSequence(plate->statusIcon, STATUS_ICON_ANM_POISON);
                    break;
                }
            }
            ClActor_SetVisible(plate->statusIcon, TRUE);
        }
    }
    if (plate->entry != POKELIST_ENTRY_EMPTY) {
        u32 msgId;

        if (plate->entry == POKELIST_ENTRY_ABLE) {
            msgId = BtlMain11_Text_NotEntered;
        } else if (plate->entry == POKELIST_ENTRY_UNABLE) {
            msgId = BtlMain11_Text_Banned;
        } else {
            msgId = plate->entry + BtlMain11_Text_First;
        }
        PokeList_PrintString(wk, plate->window, msgId, 46, 31, 0x440);
        ClActor_SetVisible(plate->hpBar, FALSE);
    } else if (showLearn == TRUE) {
        u32 msgId;

        switch (PokeList_CheckLearnMove(wk, plate->pkm, plate->index)) {
        case POKELIST_LEARN_EMPTY_SLOT:
        case POKELIST_LEARN_REPLACE_MOVE:
            msgId = BtlMain11_Text_Able;
            break;
        case POKELIST_LEARN_UNABLE:
            msgId = BtlMain11_Text_Unable;
            break;
        case POKELIST_LEARN_KNOWN:
            msgId = BtlMain11_Text_Learned_2;
            break;
        }
        PokeList_PrintString(wk, plate->window, msgId, 29, 31, 0x440);
        ClActor_SetVisible(plate->hpBar, FALSE);
    } else if (wk->param->mode == POKELIST_MODE_EVOLUTION_ITEM) {
        u32 msgId = PokeList_CanEvolveWithItem(wk, plate->pkm, wk->param->item) == FALSE ? BtlMain11_Text_Unable_2
                                                                                         : BtlMain11_Text_Able_2;

        PokeList_PrintString(wk, plate->window, msgId, 46, 31, 0x440);
        ClActor_SetVisible(plate->hpBar, FALSE);
    } else if (wk->wasMusicalMode == TRUE) {
        u32 msgId = MusicalSystem_CanJoin(plate->pkm) == TRUE ? BtlMain11_Text_Able_3 : BtlMain11_Text_Banned_2;

        PokeList_PrintString(wk, plate->window, msgId, 46, 31, 0x440);
    } else {
        WordSet *wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);

        WordSetNumber(wordSet, 0, PokeParty_GetParam(plate->pkm, PKM_PARAM_MAX_HP, NULL), 3, 0, TRUE);
        if (numbersOnly == FALSE) {
            PokeList_PrintWordSetStringSmall(wk, plate->window, wordSet, BtlMain11_Text_Empty_2, 91, 35, 0x440);
        } else {
            PokeList_DrawWordSetStringSmall(wk, plate->window, wordSet, BtlMain11_Text_Empty_2, 91, 35, 0x440);
        }
        GFL_WordSetSystemFree(wordSet);
        wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
        WordSetNumber(wordSet, 0, plate->hp, 3, 1, TRUE);
        if (numbersOnly == FALSE) {
            PokeList_PrintWordSetStringSmall(wk, plate->window, wordSet, BtlMain11_Text_Empty_3, 57, 35, 0x440);
        } else {
            PokeList_DrawWordSetStringSmall(wk, plate->window, wordSet, BtlMain11_Text_Empty_3, 57, 35, 0x440);
        }
        GFL_WordSetSystemFree(wordSet);
        if (numbersOnly == FALSE) {
            PokeList_PrintStringSmall(wk, plate->window, BtlMain11_Text_Empty_4, 82, 35, 0x440);
        } else {
            PokeList_DrawStringSmall(wk, plate->window, BtlMain11_Text_Empty_4, 82, 35, 0x440);
        }
    }
    item = PokeParty_GetParam(plate->pkm, PKM_PARAM_ITEM, NULL);
    if (item != 0) {
        ClActor_SetVisible(plate->itemIcon, TRUE);
        if (PML_ItemIsMail(item) == TRUE) {
            ClActor_SetSequence(plate->itemIcon, POKELIST_HELD_ITEM_ANM_MAIL);
        } else {
            ClActor_SetSequence(plate->itemIcon, POKELIST_HELD_ITEM_ANM_ITEM);
        }
    } else {
        ClActor_SetVisible(plate->itemIcon, FALSE);
    }
    {
        u32 color = PokeListPlate_GetHpColor(plate);
        u16 sequence = POKELIST_PARTY_ICON_ANM_DEFAULT;

        if (color == HP_GAUGE_COLOR_NONE) {
            sequence = POKELIST_PARTY_ICON_ANM_FAINTED;
        } else if (GetStatusCond(plate->pkm) != CONDITION_NONE) {
            sequence = POKELIST_PARTY_ICON_ANM_STATUS;
        } else if (color == HP_GAUGE_COLOR_RED) {
            sequence = POKELIST_PARTY_ICON_ANM_HP_RED;
        } else if (color == HP_GAUGE_COLOR_YELLOW) {
            sequence = POKELIST_PARTY_ICON_ANM_HP_YELLOW;
        } else if (color == HP_GAUGE_COLOR_GREEN) {
            sequence = POKELIST_PARTY_ICON_ANM_HP_GREEN;
        }
        ClActor_SetSequence(plate->icon, sequence);
    }
    plate->nameDirty = TRUE;
}

static void PokeListPlate_DrawHpBar(PokeListWork *wk, PokeListPlate *plate) {
    if (PokeList_IsBattle(wk) == FALSE && wk->param->mode != POKELIST_MODE_TEACH_TM) {
        u8 color = PokeListPlate_GetHpColor(plate);
        u8 width = HPGauge_GetFill(plate->hp, PokeParty_GetParam(plate->pkm, PKM_PARAM_MAX_HP, NULL), 48);
        GFLBitmap *bitmap = BmpWin_GetBitmap(plate->window);
        u8 topColor;
        u8 bottomColor;

        if (color == HP_GAUGE_COLOR_RED || color == HP_GAUGE_COLOR_NONE) {
            topColor = 14;
            bottomColor = 15;
        } else if (color == HP_GAUGE_COLOR_YELLOW) {
            topColor = 12;
            bottomColor = 13;
        } else {
            topColor = 10;
            bottomColor = 11;
        }
        GFL_BitmapFillArea(bitmap, 61, 27, width, 1, topColor);
        GFL_BitmapFillArea(bitmap, 61, 28, width, 1, bottomColor);
        ClActor_SetVisible(plate->hpBar, TRUE);
    }
}

static void PokeListPlate_DrawEgg(PokeListWork *wk, PokeListPlate *plate) {
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    StrBuf *name = GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, SPECIES_EGG);

    GFL_WordSetSetStrbuf(wordSet, 0, name, 0, 1, 2);
    PokeList_PrintWordSetString(wk, plate->window, wordSet, BtlMain11_Text_Empty, 41, 9, 0x440);
    GFL_StrBufFree(name);
    GFL_WordSetSystemFree(wordSet);
    ClActor_SetVisible(plate->hpBar, FALSE);
    ClActor_SetSequence(plate->icon, POKELIST_PARTY_ICON_ANM_DEFAULT);
    plate->nameDirty = TRUE;
}

void PokeListPlate_SetSelected(PokeListWork *wk, PokeListPlate *plate, BOOL selected) {
    if (selected == TRUE) {
        ClActorPos pos;

        if (wk->state == POKELIST_STATE_SELECT_SWAP || wk->state == POKELIST_STATE_SELECT_HP_SHARE ||
            wk->state == POKELIST_STATE_SELECT_FUSION) {
            PokeListPlate_SetPalette(wk, plate, POKELIST_PLATE_PALETTE_MARKED_SELECTED);
        } else if (PokeList_IsBattle(wk) == TRUE && plate->entry < POKELIST_ENTRY_ABLE) {
            PokeListPlate_SetPalette(wk, plate, POKELIST_PLATE_PALETTE_MARKED_SELECTED);
        } else {
            PokeListPlate_SetPalette(wk, plate, POKELIST_PLATE_PALETTE_SELECTED);
        }
        ClActor_SetSequence(plate->ball, PARTY_BALL_ANM_SELECTED);
        PokeListPlate_GetIconPos(plate, 26, 19, &pos);
        ClActor_SetSurfacePos(plate->icon, &pos, 0);
        plate->bounceTimer = 0;
    } else {
        ClActorPos pos;

        if ((wk->state == POKELIST_STATE_SELECT_SWAP && wk->selectPos == plate->index) ||
            (wk->state == POKELIST_STATE_SELECT_FUSION && wk->selectPos == plate->index) ||
            (wk->state == POKELIST_STATE_SELECT_HP_SHARE && wk->selectPos2 == plate->index)) {
            PokeListPlate_SetPalette(wk, plate, POKELIST_PLATE_PALETTE_MARKED);
        } else if (PokeList_IsBattle(wk) == TRUE && plate->entry < POKELIST_ENTRY_ABLE) {
            PokeListPlate_SetPalette(wk, plate, POKELIST_PLATE_PALETTE_MARKED);
        } else {
            PokeListPlate_SetPalette(wk, plate, POKELIST_PLATE_PALETTE_NORMAL);
        }
        ClActor_SetSequence(plate->ball, PARTY_BALL_ANM_NORMAL);
        PokeListPlate_GetIconPos(plate, 24, 16, &pos);
        ClActor_SetSurfacePos(plate->icon, &pos, 0);
    }
    plate->selected = selected;
}

// The palette of a plate: the fainted ones' palette follows each of 3 and 6
void PokeListPlate_SetPalette(PokeListWork *wk, PokeListPlate *plate, u32 palette) {
    if (palette == POKELIST_PLATE_PALETTE_NORMAL && PokeListPlate_GetHpColor(plate) == HP_GAUGE_COLOR_NONE) {
        GFL_BGSysSetScrPaletteNo(2, sPlatePos[plate->index][0] + 16, sPlatePos[plate->index][1], 16, 6,
                                 POKELIST_PLATE_PALETTE_FAINTED);
    } else if (palette == POKELIST_PLATE_PALETTE_SELECTED && PokeListPlate_GetHpColor(plate) == HP_GAUGE_COLOR_NONE) {
        GFL_BGSysSetScrPaletteNo(2, sPlatePos[plate->index][0] + 16, sPlatePos[plate->index][1], 16, 6,
                                 POKELIST_PLATE_PALETTE_SELECTED_FAINTED);
    } else {
        GFL_BGSysSetScrPaletteNo(2, sPlatePos[plate->index][0] + 16, sPlatePos[plate->index][1], 16, 6, palette);
    }
    GFL_BGSysQueueScrLoad(2);
}

// Draws the plate offset by a step of its slide, to the left for the left column and to the right for the right
void PokeListPlate_DrawSlid(PokeListWork *wk, PokeListPlate *plate, int step) {
    u8 x = sPlatePos[plate->index][0];
    u8 y = sPlatePos[plate->index][1];
    ClActorPos pos;

    if (plate->index % 2 == 0) {
        step = -step;
    }
    BmpWin_SetPosX(plate->window, step + (x + 16));
    pos.x = -(x + step) * 8;
    pos.y = -y * 8;
    ClActRenderer_SetSurfacePos(plate->renderer, 0, &pos);
    GFL_BGSysLoadScrArea(2, x + 16 + step, y, 16, 6, wk->plateScreen->rawData,
                         sPlateScreenPos[plate->index != 0 ? 1 : 0][0], sPlateScreenPos[plate->index != 0 ? 1 : 0][1],
                         32, 32);
    GFL_BGSysSetScrPaletteNo(2, x + 16 + step, y, 16, 6, POKELIST_PLATE_PALETTE_MARKED);
    BmpWin_FlushMap(plate->window);
    GFL_BGSysQueueScrLoad(1);
    GFL_BGSysQueueScrLoad(2);
}

// Clears where the plate is drawn at a step of its slide
void PokeListPlate_ClearSlid(PokeListWork *wk, PokeListPlate *plate, int step) {
    u8 x = sPlatePos[plate->index][0];
    u8 y = sPlatePos[plate->index][1];
    u32 left;

    if (plate->index % 2 == 0) {
        step = -step;
    }
    left = x + 16 + step;
    GFL_BGSysFillScrArea(2, 0, left, y, 16, 6, POKELIST_PLATE_PALETTE_MARKED);
    GFL_BGSysFillScrArea(1, 0, left, y, 16, 6, POKELIST_PLATE_PALETTE_MARKED);
}

// Shows another Pokémon on the plate, as swapping two does
void PokeListPlate_SetPkm(PokeListWork *wk, PokeListPlate *plate, PartyPkm *pkm, int step) {
    u8 x = sPlatePos[plate->index][0];
    u8 y = sPlatePos[plate->index][1];
    ClActorPos origin;
    ClActorPos pos;

    if (plate->index % 2 == 0) {
        step = -step;
    }
    plate->pkm = pkm;
    plate->nameDirty = FALSE;
    plate->isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    plate->hp = PokeParty_GetParam(plate->pkm, PKM_PARAM_HP, NULL);
    plate->targetHp = plate->hp;
    ClActor_Delete(plate->icon);
    ClActSys_FreeChars(plate->iconChars);
    GFL_BitmapFill(BmpWin_GetBitmap(plate->window), 0);
    ClActor_SetVisible(plate->itemIcon, FALSE);
    ClActor_SetVisible(plate->statusIcon, FALSE);
    origin.x = 0;
    origin.y = 0;
    ClActRenderer_SetSurfacePos(plate->renderer, 0, &origin);
    PokeListPlate_CreateIcon(wk, plate);
    pos.x = -(x + step) * 8;
    pos.y = -y * 8;
    ClActRenderer_SetSurfacePos(plate->renderer, 0, &pos);
    PokeListPlate_Draw(wk, plate, FALSE);
}

void PokeListPlate_Redraw(PokeListWork *wk, PokeListPlate *plate) {
    GFL_BitmapFill(BmpWin_GetBitmap(plate->window), 0);
    PokeListPlate_Draw(wk, plate, FALSE);
}

static void PokeListPlate_RedrawNumbers(PokeListWork *wk, PokeListPlate *plate) {
    GFL_BitmapFillArea(BmpWin_GetBitmap(plate->window), 0, 26, 128, 22, 0);
    PokeListPlate_Draw(wk, plate, TRUE);
}

// Starts moving the HP shown toward the Pokémon's HP, by a hundredth of the change each frame
void PokeListPlate_StartHpChange(PokeListWork *wk, PokeListPlate *plate) {
    plate->targetHp = PokeParty_GetParam(plate->pkm, PKM_PARAM_HP, NULL);
    wk->hpStep = (plate->targetHp - plate->hp) / 100;
    if (plate->targetHp > plate->hp) {
        wk->hpStep++;
    } else {
        wk->hpStep--;
    }
}

// Moves the HP shown, and returns TRUE once it is the Pokémon's
BOOL PokeListPlate_UpdateHpChange(PokeListWork *wk, PokeListPlate *plate) {
    if (plate->hp < plate->targetHp) {
        if (plate->hp == 0) {
            PokeListPlate_SetPalette(wk, plate, POKELIST_PLATE_PALETTE_SELECTED);
        }
        if (plate->hp + wk->hpStep > plate->targetHp) {
            plate->hp = plate->targetHp;
        } else {
            plate->hp += wk->hpStep;
        }
        PokeListPlate_RedrawNumbers(wk, plate);
        return FALSE;
    }
    if (plate->hp > plate->targetHp) {
        if (plate->hp + wk->hpStep < plate->targetHp) {
            plate->hp = plate->targetHp;
        } else {
            plate->hp += wk->hpStep;
        }
        PokeListPlate_RedrawNumbers(wk, plate);
        return FALSE;
    }
    return TRUE;
}

BOOL PokeListPlate_IsValid(PokeListWork *wk, PokeListPlate *plate) {
    if (plate->isEmpty == TRUE) {
        return FALSE;
    }
    return TRUE;
}

// Where the cursor is shown on the plate
void PokeListPlate_GetCursorPos(PokeListWork *wk, PokeListPlate *plate, ClActorPos *pos) {
    pos->x = (sPlatePos[plate->index][0] + 8) * 8;
    pos->y = (sPlatePos[plate->index][1] + 3) * 8;
}

void PokeListPlate_GetTouchRect(PokeListWork *wk, PokeListPlate *plate, TouchRect *rect) {
    rect->left = sPlatePos[plate->index][0] * 8;
    rect->top = sPlatePos[plate->index][1] * 8;
    rect->right = (sPlatePos[plate->index][0] + 16) * 8;
    rect->bottom = (sPlatePos[plate->index][1] + 6) * 8;
}

int PokeListPlate_GetEntry(PokeListPlate *plate) {
    return plate->entry;
}

void PokeListPlate_SetEntry(PokeListWork *wk, PokeListPlate *plate, int entry) {
    if (plate->entry != entry) {
        if (entry < POKELIST_ENTRY_ABLE) {
            plate->entry = entry;
        } else {
            PokeListPlate_UpdateEntry(wk, plate);
        }
        PokeListPlate_Redraw(wk, plate);
    }
}

u16 PokeListPlate_GetHp(PokeListWork *wk, PokeListPlate *plate) {
    return plate->hp;
}

BOOL PokeListPlate_IsEgg(PokeListWork *wk, PokeListPlate *plate) {
    return plate->isEgg;
}

static void PokeListPlate_UpdateEntry(PokeListWork *wk, PokeListPlate *plate) {
    if (PokeList_IsBattle(wk) == FALSE) {
        plate->entry = POKELIST_ENTRY_EMPTY;
    } else if (PokeRegulation_CheckPokemonExtraRule(wk->param->regulation, wk->param->extraRule, plate->pkm) == 0) {
        plate->entry = POKELIST_ENTRY_ABLE;
    } else {
        plate->entry = POKELIST_ENTRY_UNABLE;
    }
}

// Whether the Pokémon can join the battle's order: 0 if it can, 1 if the regulation bars it, 2 for a species and 3
// for an item already in the order, and 4 for a full order
u32 PokeListPlate_CheckEntry(PokeListWork *wk, PokeListPlate *plate) {
    u32 species;
    u32 item;
    u8 i;
    Regulation *regulation;

    if (plate->entry != POKELIST_ENTRY_ABLE) {
        return POKELIST_ENTRY_CHECK_REGULATION;
    }
    species = PokeParty_GetParam(plate->pkm, PKM_PARAM_SPECIES, NULL);
    item = PokeParty_GetParam(plate->pkm, PKM_PARAM_ITEM, NULL);
    regulation = wk->param->regulation;
    for (i = 0; i < 6; i++) {
        u8 picked = wk->param->picked[i];

        if (picked >= 1) {
            PartyPkm *other = wk->plates[picked - 1]->pkm;
            u32 otherSpecies = PokeParty_GetParam(other, PKM_PARAM_SPECIES, NULL);
            u32 otherItem = PokeParty_GetParam(other, PKM_PARAM_ITEM, NULL);

            if (species == otherSpecies && regulation->allowSameSpecies == 0) {
                return POKELIST_ENTRY_CHECK_SAME_SPECIES;
            }
            if (item == otherItem && item != 0 && regulation->allowSameItem == 0) {
                return POKELIST_ENTRY_CHECK_SAME_ITEM;
            }
        }
    }
    if (wk->enteredCount == regulation->maxCount) {
        return POKELIST_ENTRY_CHECK_FULL;
    }
    return POKELIST_ENTRY_CHECK_ALLOWED;
}

static u8 PokeListPlate_GetHpColor(PokeListPlate *plate) {
    return HPGauge_GetColor(plate->hp, PokeParty_GetParam(plate->pkm, PKM_PARAM_MAX_HP, NULL));
}

// Where the icon goes for a point of the plate
static void PokeListPlate_GetIconPos(PokeListPlate *plate, int x, int y, ClActorPos *pos) {
    ClActorPos surfacePos;

    ClActRenderer_GetSurfacePos(plate->renderer, 0, &surfacePos);
    pos->x = x - surfacePos.x;
    pos->y = y - surfacePos.y;
}
