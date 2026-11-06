#include "battle/b_bag_obj.h"
#include "battle/b_app_tool.h"
#include "battle/b_bag_item.h"
#include "battle/b_bag_main.h"
#include "battle/btlv_effect.h"
#include "battle/btlv_finger_cursor.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "nitro/gx.h"
#include "pml/item.h"
#include "system/palanm.h"
#include "system/shooter_item.h"

// The battle bag's OBJ resources and cell actors: the item icons, the Wonder Launcher's costs and energy gauges,
// overlay 285's cursor and overlay 168's finger cursor of the catching demo. The file name is our guess, after the
// ROM's b_bag_main.c and the b_bag_* files of Diamond and Pearl.

#define BBAG_RESOURCE_NONE (-1)

// How BBagObj_CreateActor creates each of the bag's actors, by index into the work's resources
typedef struct {
    u32 chars;
    u32 palette;
    u32 cellAnims;
} BBagActorSetup;

static void BBagObj_ClearResources(BBagWork *wk);
static void BBagObj_LoadItemIconRes(BBagWork *wk);
static void BBagObj_LoadItemIcon(BBagWork *wk, u16 item, u32 chars, u32 row);
static void BBagObj_LoadItemIconPalette(BBagWork *wk, u16 item, u32 row);
static void BBagObj_LoadGaugeRes(BBagWork *wk);
static void BBagObj_LoadCursorRes(BBagWork *wk);
static ClActor *BBagObj_CreateActor(BBagWork *wk, const BBagActorSetup *data);
static void BBagObj_CreateActors(BBagWork *wk);
static void BBagObj_ShowAt(ClActor *actor, const ClActorPos *pos);
static void BBagObj_ShowPockets(BBagWork *wk);
static void BBagObj_ShowItemList(BBagWork *wk);
static void BBagObj_ShowItem(BBagWork *wk);
static void BBagObj_InitCursor(BBagWork *wk);
static void BBagObj_ExitCursor(BBagWork *wk);
static void BBagObj_CreateFingerCursor(BBagWork *wk);
static void BBagObj_DeleteFingerCursor(BBagWork *wk);
static void BBagObj_ShowFingerCursor(BBagWork *wk, s16 x, s16 y);
static void BBagObj_HideFingerCursor(BBagWork *wk);
static void BBagObj_ShowCost(BBagWork *wk, u16 actor, u8 cost, u8 spent, const ClActorPos *pos);

// Declared in the order that MWCC's sort by size lays out in address order (rodata_order.py)

// The last used item on page 0
static const ClActorPos sBBagLastItemPos = { 0x24, 0xb4 };
// The item's cost on page 2
static const ClActorPos sBBagChosenCostPos = { 0xa8, 0x28 };
// The item on page 2
static const ClActorPos sBBagChosenItemPos = { 0x28, 0x2c };
// Page 1: the items, then their costs, then the energy
static const ClActorPos sBBagItemListActorPositions[13] = {
    { 0x24, 0x2d }, { 0xa4, 0x2d }, { 0x24, 0x5d }, { 0xa4, 0x5d }, { 0x24, 0x8d }, { 0xa4, 0x8d }, { 0x34, 0x29 },
    { 0xb4, 0x29 }, { 0x34, 0x59 }, { 0xb4, 0x59 }, { 0x34, 0x89 }, { 0xb4, 0x89 }, { 0x64, 0xac },
};

static const BBagActorSetup sBBagActorSetups[BBAG_ACTOR_MAX] = {
    { 0, 0, BBAG_CELL_RES_ITEM },
    { 1, 1, BBAG_CELL_RES_ITEM },
    { 2, 2, BBAG_CELL_RES_ITEM },
    { 3, 3, BBAG_CELL_RES_ITEM },
    { 4, 4, BBAG_CELL_RES_ITEM },
    { 5, 5, BBAG_CELL_RES_ITEM },
    { BBAG_RES_SHOWN_ITEM, BBAG_RES_SHOWN_ITEM, BBAG_CELL_RES_ITEM },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
    { BBAG_RES_GAUGE, BBAG_RES_GAUGE, BBAG_CELL_RES_GAUGE },
};

void BBagObj_Init(BBagWork *wk) {
    BBagObj_ClearResources(wk);
    BBagObj_LoadItemIconRes(wk);
    BBagObj_LoadGaugeRes(wk);
    BBagObj_LoadCursorRes(wk);
    BBagObj_CreateActors(wk);
    BBagObj_InitCursor(wk);
    BBagObj_CreateFingerCursor(wk);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void BBagObj_ClearResources(BBagWork *wk) {
    u32 i;

    for (i = 0; i < NELEMS(wk->charRes); i++) {
        wk->charRes[i] = BBAG_RESOURCE_NONE;
    }
    for (i = 0; i < NELEMS(wk->plttRes); i++) {
        wk->plttRes[i] = BBAG_RESOURCE_NONE;
    }
    for (i = 0; i < NELEMS(wk->cellRes); i++) {
        wk->cellRes[i] = BBAG_RESOURCE_NONE;
    }
}

static void BBagObj_LoadItemIconRes(BBagWork *wk) {
    ArcTool *arc;
    u16 item;
    u32 charType;
    u32 plttType;
    u16 i;

    if (wk->param->mode == BBAG_MODE_SHOOTER) {
        item = ShooterItem_GetItem(0);
        charType = ITEM_FILE_LIST_ICON_CHAR;
        plttType = ITEM_FILE_LIST_ICON_PLTT;
    } else {
        charType = ITEM_FILE_ICON_CHAR;
        item = ITEM_MASTER_BALL;
        plttType = ITEM_FILE_ICON_PLTT;
    }
    arc = GFL_ArcSysCreateFileHandle(PML_ItemGetIconArcID(), HEAPID_TAIL(wk->param->heapId));
    for (i = 0; i < 7; i++) {
        wk->charRes[i] =
            ClActSys_LoadChars(arc, GetItemGraphicsDatID(item, charType), FALSE, CLACT_VRAM_SUB, wk->param->heapId);
        wk->plttRes[i] =
            ClActSys_LoadPltt(arc, GetItemGraphicsDatID(item, plttType), CLACT_VRAM_SUB, i * 0x20, wk->param->heapId);
    }
    wk->cellRes[BBAG_CELL_RES_ITEM] =
        ClActSys_LoadCellAnims(arc, PML_ItemGetIconCellDatID(), PML_ItemGetIconAnimDatID(), wk->param->heapId);
    GFL_ArcToolFree(arc);
}

static void BBagObj_LoadItemIcon(BBagWork *wk, u16 item, u32 chars, u32 row) {
    NNSG2dCharacterData *charData;
    BBagParam *param = wk->param;
    ArcTool *arc;
    void *file;
    u32 charType;
    u32 plttType;

    if (param->mode == BBAG_MODE_SHOOTER) {
        charType = ITEM_FILE_LIST_ICON_CHAR;
        plttType = ITEM_FILE_LIST_ICON_PLTT;
    } else {
        charType = ITEM_FILE_ICON_CHAR;
        plttType = ITEM_FILE_ICON_PLTT;
    }
    arc = GFL_ArcSysCreateFileHandle(PML_ItemGetIconArcID(), HEAPID_TAIL(param->heapId));
    file = GFL_G2DIOReadOBJNCGRArc(arc, GetItemGraphicsDatID(item, charType), FALSE, &charData, wk->param->heapId);
    ClActSys_ReplaceCharData(wk->charRes[chars], charData);
    GFL_HeapFree(file);
    PaletteFade_LoadArcNCLR(wk->paletteFade, arc, GetItemGraphicsDatID(item, plttType), wk->param->heapId,
                            PALFADE_BUFFER_SUB_OBJ, 0x20, row * 16);
    GFL_ArcToolFree(arc);
}

static void BBagObj_LoadItemIconPalette(BBagWork *wk, u16 item, u32 row) {
    BBagParam *param = wk->param;

    if (param->mode == BBAG_MODE_SHOOTER) {
        GFL_BGSysLoadNCLRDefault(PML_ItemGetIconArcID(), GetItemGraphicsDatID(item, ITEM_FILE_LIST_ICON_PLTT),
                                 PALTYPE_SUB_OBJ, row * 0x20, 0x20, param->heapId);
    } else {
        GFL_BGSysLoadNCLRDefault(PML_ItemGetIconArcID(), GetItemGraphicsDatID(item, ITEM_FILE_ICON_PLTT),
                                 PALTYPE_SUB_OBJ, row * 0x20, 0x20, param->heapId);
    }
}

static void BBagObj_LoadGaugeRes(BBagWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_B_BAG, HEAPID_TAIL(wk->param->heapId));

    wk->charRes[BBAG_RES_GAUGE] = ClActSys_LoadChars(arc, 14, TRUE, CLACT_VRAM_SUB, wk->param->heapId);
    wk->plttRes[BBAG_RES_GAUGE] = ClActSys_LoadPltt(arc, 15, CLACT_VRAM_SUB, 0xe0, wk->param->heapId);
    wk->cellRes[BBAG_CELL_RES_GAUGE] = ClActSys_LoadCellAnims(arc, 16, 17, wk->param->heapId);
    PaletteFade_LoadArcNCLR(wk->paletteFade, arc, 15, wk->param->heapId, PALFADE_BUFFER_SUB_OBJ, 0x20, 0x70);
    GFL_ArcToolFree(arc);
}

static void BBagObj_LoadCursorRes(BBagWork *wk) {
    NNSG2dPaletteData *palette;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_BTLV, HEAPID_TAIL(wk->param->heapId));
    void *file;

    wk->charRes[BBAG_RES_CURSOR] = ClActSys_LoadChars(arc, 424, FALSE, CLACT_VRAM_SUB, wk->param->heapId);
    wk->plttRes[BBAG_RES_CURSOR] = ClActSys_LoadPlttRange(arc, 421, CLACT_VRAM_SUB, 0x100, 4, 1, wk->param->heapId);
    wk->cellRes[BBAG_CELL_RES_CURSOR] = ClActSys_LoadCellAnims(arc, 425, 426, wk->param->heapId);
    file = GFL_G2DIOReadNCLRArc(arc, 421, &palette, HEAPID_TAIL(wk->param->heapId));
    PaletteFade_LoadData(wk->paletteFade, (u16 *)palette->pRawData + 4 * 16, PALFADE_BUFFER_SUB_OBJ, 0x80, 0x20);
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
}

static ClActor *BBagObj_CreateActor(BBagWork *wk, const BBagActorSetup *data) {
    ClActorSetup setup;

    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.priority = 1;
    setup.bgPriority = 1;
    return ClActor_Create(wk->actorUnit, wk->charRes[data->chars], wk->plttRes[data->palette],
                          wk->cellRes[data->cellAnims], &setup, CLACT_SURFACE_SUB, wk->param->heapId);
}

static void BBagObj_CreateActors(BBagWork *wk) {
    u32 i;

    wk->actorUnit = ClActUnit_Create(BBAG_ACTOR_MAX + 4, 0, wk->param->heapId);
    for (i = 0; i < NELEMS(wk->actors); i++) {
        wk->actors[i] = BBagObj_CreateActor(wk, &sBBagActorSetups[i]);
    }
}

void BBagObj_Exit(BBagWork *wk) {
    u32 i;

    BBagObj_DeleteFingerCursor(wk);
    for (i = 0; i < NELEMS(wk->actors); i++) {
        ClActor_Delete(wk->actors[i]);
    }
    BBagObj_ExitCursor(wk);
    for (i = 0; i < NELEMS(wk->charRes); i++) {
        if (wk->charRes[i] != BBAG_RESOURCE_NONE) {
            ClActSys_FreeChars(wk->charRes[i]);
        }
    }
    for (i = 0; i < NELEMS(wk->plttRes); i++) {
        if (wk->plttRes[i] != BBAG_RESOURCE_NONE) {
            ClActSys_FreePltt(wk->plttRes[i]);
        }
    }
    for (i = 0; i < NELEMS(wk->cellRes); i++) {
        if (wk->cellRes[i] != BBAG_RESOURCE_NONE) {
            ClActSys_FreeCellAnims(wk->cellRes[i]);
        }
    }
    ClActUnit_Delete(wk->actorUnit);
}

static void BBagObj_ShowAt(ClActor *actor, const ClActorPos *pos) {
    ClActor_SetVisible(actor, TRUE);
    ClActor_SetSurfacePos(actor, pos, CLACT_SURFACE_SUB);
}

void BBagObj_SetPage(BBagWork *wk, u8 page) {
    u32 i;

    for (i = 0; i < NELEMS(wk->actors); i++) {
        ClActor_SetVisible(wk->actors[i], FALSE);
    }
    BBagObj_HideFingerCursor(wk);
    switch (page) {
    case BBAG_PAGE_POCKETS:
        BBagObj_ShowPockets(wk);
        break;
    case BBAG_PAGE_ITEMS:
        BBagObj_ShowItemList(wk);
        break;
    case BBAG_PAGE_USE:
        BBagObj_ShowItem(wk);
        break;
    }
}

static void BBagObj_ShowPockets(BBagWork *wk) {
    if (wk->lastItem != ITEM_NONE) {
        BBagObj_LoadItemIcon(wk, wk->lastItem, BBAG_RES_SHOWN_ITEM, BBAG_RES_SHOWN_ITEM);
        BBagObj_ShowAt(wk->actors[BBAG_ACTOR_SHOWN_ITEM], &sBBagLastItemPos);
    }
    if (wk->param->mode == BBAG_MODE_CAPTURE_DEMO) {
        BBagObj_ShowFingerCursor(wk, 192, 24);
    }
}

static void BBagObj_ShowItemList(BBagWork *wk) {
    u32 i;

    for (i = 0; i < 6; i++) {
        u16 item = BBagItem_GetSlotItem(wk, i);
        if (item != 0) {
            BBagObj_LoadItemIcon(wk, item, i, i);
            BBagObj_LoadItemIconPalette(wk, item, i);
            BBagObj_ShowAt(wk->actors[BBAG_ACTOR_ITEM + i], &sBBagItemListActorPositions[i]);
            if (wk->param->mode == BBAG_MODE_SHOOTER) {
                BBagObj_ShowCost(wk, BBAG_ACTOR_COST + i * 8, BBagItem_GetShooterCost(item), 0,
                                 &sBBagItemListActorPositions[i + 6]);
            }
        }
    }
    if (wk->param->mode == BBAG_MODE_SHOOTER) {
        BBagObj_ShowCost(wk, BBAG_ACTOR_ENERGY, wk->param->shooterEnergy, wk->param->shooterSpent,
                         &sBBagItemListActorPositions[i + 6]);
    }
    if (wk->param->mode == BBAG_MODE_CAPTURE_DEMO) {
        BBagObj_ShowFingerCursor(wk, 64, 16);
    }
}

static void BBagObj_ShowItem(BBagWork *wk) {
    u16 item = BBagItem_GetSlotItem(wk, wk->param->rows[wk->pocket]);

    BBagObj_LoadItemIcon(wk, item, BBAG_RES_SHOWN_ITEM, BBAG_RES_SHOWN_ITEM);
    BBagObj_ShowAt(wk->actors[BBAG_ACTOR_SHOWN_ITEM], &sBBagChosenItemPos);
    if (wk->param->mode == BBAG_MODE_SHOOTER) {
        BBagObj_ShowCost(wk, BBAG_ACTOR_COST, BBagItem_GetShooterCost(item), 0, &sBBagChosenCostPos);
    }
    if (wk->param->mode == BBAG_MODE_CAPTURE_DEMO) {
        BBagObj_ShowFingerCursor(wk, 104, 152);
    }
}

static void BBagObj_InitCursor(BBagWork *wk) {
    BAppCursor_CreateActors(wk->cursor, wk->actorUnit, wk->charRes[BBAG_RES_CURSOR], wk->plttRes[BBAG_RES_CURSOR],
                            wk->cellRes[BBAG_CELL_RES_CURSOR]);
    BAppCursor_SetVisible(wk->cursor, FALSE);
}

static void BBagObj_ExitCursor(BBagWork *wk) {
    BAppCursor_DeleteActors(wk->cursor);
}

static void BBagObj_CreateFingerCursor(BBagWork *wk) {
    wk->fingerCursor = BtlvFingerCursor_Create(wk->paletteFade, 9, wk->param->heapId);
}

static void BBagObj_DeleteFingerCursor(BBagWork *wk) {
    BtlvFingerCursor_Delete(wk->fingerCursor);
}

static void BBagObj_ShowFingerCursor(BBagWork *wk, s16 x, s16 y) {
    BtlvFingerCursor_Start(wk->fingerCursor, x, y, 2, 6, 20);
}

static void BBagObj_HideFingerCursor(BBagWork *wk) {
    BtlvFingerCursor_RemoveActor(wk->fingerCursor);
}

static void BBagObj_ShowCost(BBagWork *wk, u16 actor, u8 cost, u8 spent, const ClActorPos *pos) {
    ClActorPos p;
    s8 c;
    s8 s;
    u16 i;

    BBagObj_ShowAt(wk->actors[actor], pos);
    ClActor_SetSequence(wk->actors[actor], cost);
    actor++;
    c = cost;
    s = spent;
    p = *pos;
    p.x += 12;
    for (i = 0; i < 7; i++) {
        if (c <= 0) {
            if (s <= 0) {
                ClActor_SetSequence(wk->actors[actor], BBAG_GAUGE_ANM_EMPTY);
            } else if (s == 1) {
                ClActor_SetSequence(wk->actors[actor], BBAG_GAUGE_ANM_SPENT_ONE);
                s -= 1;
            } else {
                ClActor_SetSequence(wk->actors[actor], BBAG_GAUGE_ANM_SPENT_TWO);
                s -= 2;
            }
        } else if (c == 1) {
            if (s == 0) {
                ClActor_SetSequence(wk->actors[actor], BBAG_GAUGE_ANM_UNSPENT_ONE);
            } else if (s > 0) {
                ClActor_SetSequence(wk->actors[actor], BBAG_GAUGE_ANM_UNSPENT_ONE_SPENT_ONE);
                s -= 1;
            }
            c -= 1;
        } else {
            ClActor_SetSequence(wk->actors[actor], BBAG_GAUGE_ANM_UNSPENT_TWO);
            c -= 2;
        }
        BBagObj_ShowAt(wk->actors[actor], &p);
        p.x += 8;
        actor++;
    }
}
