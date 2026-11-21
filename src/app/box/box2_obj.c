#include "types.h"
#include "app/box2_bmp.h"
#include "app/box2_main.h"
#include "app/box2_obj.h"
#include "app/box2_ui.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "pml/item.h"
#include "pml/personal.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "system/app_menu_common.h"
#include "system/bgwinfrm.h"
#include "system/bmp_oam.h"
#include "system/cursor_move.h"

// The PC box's actors: the cursor, the Pokémon and item icons, the tray icons of the box list and the buttons. The ROM
// doesn't name this file; box2_obj.c is a guess after box2_main.c. Names are ours

// No resource loaded
#define BOX2_RES_NONE 0xffffffff

// An actor to create: where, its resources and its surface
typedef struct {
    ClActorSetup setup;
    u32 chr;
    u32 pal;
    u32 cell;
    // A palette set once the actor is created, by the code that uses it
    u16 palette;
    u16 surface;
} Box2ActorData;

// An area of the screen, with its right and bottom edges in it
typedef struct {
    s32 left;
    s32 top;
    s32 right;
    s32 bottom;
} Box2Rect;

// Character-resource slots.
enum {
    BOX2_CHAR_POKE_ICON_FIRST = 0,
    BOX2_CHAR_POKE_GRA_FIRST = 96,
    BOX2_CHAR_POKE_GRA_SECOND = 97,
    BOX2_CHAR_ITEM_ICON = 98,
    BOX2_CHAR_TYPE_ICON_FIRST = 99,
    BOX2_CHAR_TRAY_ICON_FIRST = 116,
    BOX2_CHAR_BOX_UI = 122,
    BOX2_CHAR_SUMMARY_BUTTON = 123,
    BOX2_CHAR_TOUCH_BAR = 124,
    BOX2_CHAR_MARK_MAIN = 125,
    BOX2_CHAR_MARK_SUB = 126,
    BOX2_CHAR_POKERUS = 127,
    BOX2_CHAR_SEARCH_BUTTON = 128,
};

// Palette-resource slots.
enum {
    BOX2_PAL_POKE_ICON = 0,
    BOX2_PAL_POKE_GRA_FIRST = 1,
    BOX2_PAL_POKE_GRA_SECOND = 2,
    BOX2_PAL_ITEM_ICON = 3,
    BOX2_PAL_TYPE_ICON = 4,
    BOX2_PAL_TRAY_ICON = 5,
    BOX2_PAL_BOX_UI = 6,
    BOX2_PAL_TOUCH_BAR = 7,
    BOX2_PAL_MARK_MAIN = 8,
    BOX2_PAL_MARK_SUB = 9,
    BOX2_PAL_POKERUS = 10,
};

// Cell-animation resource slots.
enum {
    BOX2_CELL_POKE_ICON = 0,
    BOX2_CELL_POKE_GRA_FIRST = 1,
    BOX2_CELL_POKE_GRA_SECOND = 2,
    BOX2_CELL_ITEM_ICON = 3,
    BOX2_CELL_TYPE_ICON = 4,
    BOX2_CELL_TRAY_ICON = 5,
    BOX2_CELL_BOX_UI = 6,
    BOX2_CELL_SUMMARY_BUTTON = 7,
    BOX2_CELL_TOUCH_BAR = 8,
    BOX2_CELL_MARK_MAIN = 9,
    BOX2_CELL_MARK_SUB = 10,
    BOX2_CELL_POKERUS = 11,
    BOX2_CELL_SEARCH_BUTTON = 12,
};

static void CreateClActSys(Box2AppWork *app);
static void LoadResources(Box2AppWork *app);
static void FreeResources(Box2AppWork *app);
static void CreateActors(Box2SysWork *syswk);
static ClActor *CreateActor(Box2AppWork *app, const Box2ActorData *data);
static void DeleteBoxActor(Box2AppWork *app, u32 id);
static void DeleteBoxActors(Box2AppWork *app);
static void SetBgPriority(Box2AppWork *app, u32 id, u32 priority);
static void SetSoftPriority(Box2AppWork *app, u32 id, u32 priority);
static void SetBlend(Box2AppWork *app, u32 id, BOOL blend);
static void SetPalette(Box2AppWork *app, u32 id, u32 palette);
static void LoadBoxResources(Box2AppWork *app);
static void LoadUiResources(Box2AppWork *app);
static void LoadPokeIconResources(Box2AppWork *app);
static void CreatePokeIconActors(Box2AppWork *app);
static void *ReadPokeIconChar(Box2AppWork *app, BoxPkm *pkm, NNSG2dCharacterData **chr);
static BOOL MatchesSearch(Box2SearchParam *search, BoxPkm *pkm);
static u32 GetPokeIconPalette(Box2SysWork *syswk, BoxPkm *pkm);
static void TransferIconChar(Box2AppWork *app, void *chr, u32 id, u32 palette);
static void SetPokeIcon(Box2SysWork *syswk, BoxPkm *pkm, u32 id);
static void SetIconsBlend(Box2SysWork *syswk, BOOL blend, u32 start, u32 end, BOOL check);
static void SetIconBlendChecked(Box2SysWork *syswk, u32 pos);
static void ClearPokeGraResources(Box2AppWork *app);
static void LoadInfoResources(Box2AppWork *app);
static void CreateTypeIconActors(Box2AppWork *app);
static void ShowTypeIcons(Box2AppWork *app, u32 type1, u32 type2);
static void LoadItemIconPalette(Box2AppWork *app);
static void CreateFixedActors(Box2SysWork *syswk);
static void CreateTrayIconActors(Box2AppWork *app);
static void PlaceTrayIcons(Box2SysWork *syswk);
static void SetTrayIconChar(Box2SysWork *syswk, u32 tray, u32 id);
static void RefreshTrayIconRow(Box2SysWork *syswk, u32 tray, u32 row);
static void ReplaceTrayIconColor(Box2SysWork *syswk, u8 *chr, u32 wallpaper, u32 color, u32 size);
static void DrawTrayIconPokes(Box2SysWork *syswk, u32 tray, u8 *chr);
static void PutBoxNameFrame(Box2SysWork *syswk, u32 index);
static void CreateFontOams(Box2AppWork *app);
static void DeleteFontOams(Box2AppWork *app);
static void InitRangeSelect(Box2SysWork *syswk);
static void SortPair(s32 *a, s32 *b);
static void SetRangeRect(Box2SysWork *syswk, s32 left, s32 top, s32 right, s32 bottom);
static s32 Min4(s32 a, s32 b, s32 c, s32 d);
static s32 Max4(s32 a, s32 b, s32 c, s32 d);
static void GetTrayPokeRect(int pos, Box2Rect *rect);
static void GetPartyPokeRect(int pos, Box2Rect *rect);
static void GetPartyPokeRectCompact(int pos, Box2Rect *rect);
static void GetTrayRangeCorners(int start, int end, s32 *x1, s32 *y1, s32 *x2, s32 *y2);
static void GetPartyRangeCorners(int start, int end, s32 *x1, s32 *y1, s32 *x2, s32 *y2);
static BOOL IsInPickedRange(Box2SysWork *syswk, u32 pos);

// Where the copies of the held item's icon that outline it are, around the icon
static const s8 sItemOutlineX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
static const s8 sItemOutlineY[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };

// Where the box name's frame is, by its position
static const s16 sBoxNameY[4] = { 52, 86, 120, 154 };

// The animations of the three buttons, by set
static const u8 sButtonAnims[3][3] = {
    { 19, 20, 22 },
    { 18, 21, 22 },
    { 18, 20, 23 },
};

// Where the party's icons are on the party's frame
static const u8 sPartyFramePokePos[PARTY_SIZE][2] = {
    { 24, 16 }, { 64, 24 }, { 24, 48 }, { 64, 56 }, { 24, 80 }, { 64, 88 },
};

// The type icons on the upper screen
static const Box2ActorData sTypeIconData = {
    { 160, 48, 0, 0, 1 }, BOX2_CHAR_TYPE_ICON_FIRST, BOX2_PAL_TYPE_ICON, BOX2_CELL_TYPE_ICON, 0, CLACT_SURFACE_SUB,
};

// The first of the tray's Pokémon icons
static const Box2ActorData sPokeIconData = {
    { 24, 48, 0, 10, 3 }, BOX2_CHAR_POKE_ICON_FIRST, BOX2_PAL_POKE_ICON, BOX2_CELL_POKE_ICON, 0, CLACT_SURFACE_MAIN,
};

// Where the party's icons are on the lower screen
static const ClActorPos sPartyPokePos[PARTY_SIZE] = {
    { 40, 208 }, { 80, 216 }, { 40, 240 }, { 80, 248 }, { 40, 272 }, { 80, 280 },
};

// The dots that run around the picked range
static const Box2ActorData sRangeDotData = {
    { 220, 120, 25, 19, 0 }, BOX2_CHAR_BOX_UI, BOX2_PAL_BOX_UI, BOX2_CELL_BOX_UI, 0, CLACT_SURFACE_MAIN,
};

// The tray icons of the box list
static const Box2ActorData sTrayIconData = {
    { 300, 0, 0, 80, 1 }, BOX2_CHAR_TRAY_ICON_FIRST, BOX2_PAL_TRAY_ICON, BOX2_CELL_TRAY_ICON, 0, CLACT_SURFACE_MAIN,
};

static const ClActSysSetup sClActSysSetup = {
    0, 0, 0, 512, 4, 124, 4, 124, 0, 217, 11, 101, 0, 16, 16,
};

// The color of the tray icons' background, by wallpaper
static const u8 sTrayIconColors[24] = {
    0x1b, 0x17, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
    0x26, 0x27, 0x10, 0x28, 0x18, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
};

// The actors that are created from one setup each, by their ID
// clang-format off
static const Box2ActorData sActorData[32] = {
    { { 12, 28, 1, 0, 3 }, BOX2_CHAR_BOX_UI, BOX2_PAL_BOX_UI, BOX2_CELL_BOX_UI, 0, CLACT_SURFACE_MAIN },
    { { 156, 28, 3, 0, 3 }, BOX2_CHAR_BOX_UI, BOX2_PAL_BOX_UI, BOX2_CELL_BOX_UI, 0, CLACT_SURFACE_MAIN },
    { { 0, -21, 0, 6, 1 }, BOX2_CHAR_BOX_UI, BOX2_PAL_BOX_UI, BOX2_CELL_BOX_UI, 0, CLACT_SURFACE_MAIN },
    { { 128, -40, 5, 40, 0 }, BOX2_CHAR_BOX_UI, BOX2_PAL_BOX_UI, BOX2_CELL_BOX_UI, 0, CLACT_SURFACE_MAIN },
    { { 128, 128, 6, 0, 0 }, BOX2_CHAR_BOX_UI, BOX2_PAL_BOX_UI, BOX2_CELL_BOX_UI, 0, CLACT_SURFACE_MAIN },
    { { 128, 160, 9, 0, 0 }, BOX2_CHAR_BOX_UI, BOX2_PAL_BOX_UI, BOX2_CELL_BOX_UI, 0, CLACT_SURFACE_MAIN },
    { { 232, 168, 1, 64, 0 }, BOX2_CHAR_TOUCH_BAR, BOX2_PAL_TOUCH_BAR, BOX2_CELL_TOUCH_BAR, 0, CLACT_SURFACE_MAIN },
    { { 200, 168, 0, 64, 0 }, BOX2_CHAR_TOUCH_BAR, BOX2_PAL_TOUCH_BAR, BOX2_CELL_TOUCH_BAR, 0, CLACT_SURFACE_MAIN },
    { { 112, 168, 0, 64, 0 }, BOX2_CHAR_SUMMARY_BUTTON, BOX2_PAL_TOUCH_BAR, BOX2_CELL_SUMMARY_BUTTON, 3, CLACT_SURFACE_MAIN },
    { { 32, 7, 19, 100, 0 }, BOX2_CHAR_BOX_UI, BOX2_PAL_BOX_UI, BOX2_CELL_BOX_UI, 0, CLACT_SURFACE_MAIN },
    { { 84, 7, 20, 100, 0 }, BOX2_CHAR_BOX_UI, BOX2_PAL_BOX_UI, BOX2_CELL_BOX_UI, 0, CLACT_SURFACE_MAIN },
    { { 136, 7, 22, 100, 0 }, BOX2_CHAR_BOX_UI, BOX2_PAL_BOX_UI, BOX2_CELL_BOX_UI, 0, CLACT_SURFACE_MAIN },
    { { 0, 0, 0, 0, 0 }, BOX2_CHAR_ITEM_ICON, BOX2_PAL_ITEM_ICON, BOX2_CELL_ITEM_ICON, 0, CLACT_SURFACE_MAIN },
    { { 200, 56, 0, 10, 1 }, BOX2_CHAR_POKE_GRA_FIRST, BOX2_PAL_POKE_GRA_FIRST, BOX2_CELL_POKE_GRA_FIRST, 0, CLACT_SURFACE_SUB },
    { { 200, 56, 0, 10, 1 }, BOX2_CHAR_POKE_GRA_SECOND, BOX2_PAL_POKE_GRA_SECOND, BOX2_CELL_POKE_GRA_SECOND, 0, CLACT_SURFACE_SUB },
    { { 192, 200, 0, 10, 1 }, BOX2_CHAR_MARK_MAIN, BOX2_PAL_MARK_MAIN, BOX2_CELL_MARK_MAIN, 0, CLACT_SURFACE_MAIN },
    { { 224, 200, 0, 10, 1 }, BOX2_CHAR_MARK_MAIN, BOX2_PAL_MARK_MAIN, BOX2_CELL_MARK_MAIN, 0, CLACT_SURFACE_MAIN },
    { { 192, 224, 0, 10, 1 }, BOX2_CHAR_MARK_MAIN, BOX2_PAL_MARK_MAIN, BOX2_CELL_MARK_MAIN, 0, CLACT_SURFACE_MAIN },
    { { 224, 224, 0, 10, 1 }, BOX2_CHAR_MARK_MAIN, BOX2_PAL_MARK_MAIN, BOX2_CELL_MARK_MAIN, 0, CLACT_SURFACE_MAIN },
    { { 192, 248, 0, 10, 1 }, BOX2_CHAR_MARK_MAIN, BOX2_PAL_MARK_MAIN, BOX2_CELL_MARK_MAIN, 0, CLACT_SURFACE_MAIN },
    { { 224, 248, 0, 10, 1 }, BOX2_CHAR_MARK_MAIN, BOX2_PAL_MARK_MAIN, BOX2_CELL_MARK_MAIN, 0, CLACT_SURFACE_MAIN },
    { { 200, 103, 0, 10, 1 }, BOX2_CHAR_MARK_SUB, BOX2_PAL_MARK_SUB, BOX2_CELL_MARK_SUB, 0, CLACT_SURFACE_SUB },
    { { 208, 103, 0, 10, 1 }, BOX2_CHAR_MARK_SUB, BOX2_PAL_MARK_SUB, BOX2_CELL_MARK_SUB, 0, CLACT_SURFACE_SUB },
    { { 216, 103, 0, 10, 1 }, BOX2_CHAR_MARK_SUB, BOX2_PAL_MARK_SUB, BOX2_CELL_MARK_SUB, 0, CLACT_SURFACE_SUB },
    { { 224, 103, 0, 10, 1 }, BOX2_CHAR_MARK_SUB, BOX2_PAL_MARK_SUB, BOX2_CELL_MARK_SUB, 0, CLACT_SURFACE_SUB },
    { { 232, 103, 0, 10, 1 }, BOX2_CHAR_MARK_SUB, BOX2_PAL_MARK_SUB, BOX2_CELL_MARK_SUB, 0, CLACT_SURFACE_SUB },
    { { 240, 103, 0, 10, 1 }, BOX2_CHAR_MARK_SUB, BOX2_PAL_MARK_SUB, BOX2_CELL_MARK_SUB, 0, CLACT_SURFACE_SUB },
    { { 160, 103, 12, 10, 1 }, BOX2_CHAR_MARK_SUB, BOX2_PAL_MARK_SUB, BOX2_CELL_MARK_SUB, 0, CLACT_SURFACE_SUB },
    { { 168, 103, 13, 10, 1 }, BOX2_CHAR_MARK_SUB, BOX2_PAL_MARK_SUB, BOX2_CELL_MARK_SUB, 0, CLACT_SURFACE_SUB },
    { { 116, 104, 0, 10, 1 }, BOX2_CHAR_POKERUS, BOX2_PAL_POKERUS, BOX2_CELL_POKERUS, 0, CLACT_SURFACE_SUB },
    { { 168, 168, 0, 64, 0 }, BOX2_CHAR_SEARCH_BUTTON, BOX2_PAL_TOUCH_BAR, BOX2_CELL_SEARCH_BUTTON, 0, CLACT_SURFACE_MAIN },
    { { 0, 0, 24, 50, 0 }, BOX2_CHAR_BOX_UI, BOX2_PAL_BOX_UI, BOX2_CELL_BOX_UI, 0, CLACT_SURFACE_MAIN },
};

// clang-format on

void Box2Obj_Init(Box2SysWork *syswk) {
    CreateClActSys(syswk->app);
    LoadResources(syswk->app);
    CreateActors(syswk);
    Box2Obj_SetButtons(syswk, syswk->backButtonState, syswk->closeButtonState, syswk->summaryButtonState,
                       syswk->searchButtonState);
    CreateFontOams(syswk->app);
    InitRangeSelect(syswk);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

void Box2Obj_Exit(Box2AppWork *app) {
    DeleteFontOams(app);
    DeleteBoxActors(app);
    FreeResources(app);
    ClActSys_Delete();
}

static void CreateClActSys(Box2AppWork *app) {
    ClActSysSetup setup = sClActSysSetup;
    ClActSys_Create(&setup, Box2Main_GetVramBanks(), HEAPID_BOX2_APP);
}

static void LoadResources(Box2AppWork *app) {
    LoadBoxResources(app);
    LoadUiResources(app);
    LoadPokeIconResources(app);
    LoadItemIconPalette(app);
    ClearPokeGraResources(app);
    LoadInfoResources(app);
}

static void FreeResources(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < NELEMS(app->chrRes); i++) {
        if (app->chrRes[i] != BOX2_RES_NONE) {
            ClActSys_FreeChars(app->chrRes[i]);
        }
    }
    for (i = 0; i < NELEMS(app->palRes); i++) {
        if (app->palRes[i] != BOX2_RES_NONE) {
            ClActSys_FreePltt(app->palRes[i]);
        }
    }
    for (i = 0; i < NELEMS(app->cellRes); i++) {
        if (app->cellRes[i] != BOX2_RES_NONE) {
            ClActSys_FreeCellAnims(app->cellRes[i]);
        }
    }
}

static void CreateActors(Box2SysWork *syswk) {
    syswk->app->clunit = ClActUnit_Create(233, 0, HEAPID_BOX2_APP);
    CreateTrayIconActors(syswk->app);
    CreatePokeIconActors(syswk->app);
    CreateFixedActors(syswk);
    CreateTypeIconActors(syswk->app);
}

static ClActor *CreateActor(Box2AppWork *app, const Box2ActorData *data) {
    ClActor *actor = ClActor_Create(app->clunit, app->chrRes[data->chr], app->palRes[data->pal],
                                    app->cellRes[data->cell], &data->setup, data->surface, HEAPID_BOX2_APP);
    ClActor_SetDrawAnim(actor, FALSE);
    return actor;
}

static void DeleteBoxActor(Box2AppWork *app, u32 id) {
    if (app->actors[id] != NULL) {
        ClActor_Delete(app->actors[id]);
        app->actors[id] = NULL;
    }
}

static void DeleteBoxActors(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < NELEMS(app->actors); i++) {
        DeleteBoxActor(app, i);
    }
    ClActUnit_Delete(app->clunit);
}

void Box2Obj_UpdateAnim(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < BOX2_ACTOR_TRAY_ICON; i++) {
        if (app->actors[i] != NULL && ClActor_IsAutoAnim(app->actors[i]) != TRUE) {
            ClActor_SetAnimSpeed(app->actors[i], FX32_ONE);
        }
    }
    ClActSys_Draw();
}

void Box2Obj_SetAnim(Box2AppWork *app, u32 id, u32 anim) {
    ClActor_SetAnimTime(app->actors[id], 0);
    ClActor_SetSequence(app->actors[id], anim);
}

void Box2Obj_SetAnimAndPlay(Box2AppWork *app, u32 id, u32 anim) {
    Box2Obj_SetAnim(app, id, anim);
    ClActor_SetAutoAnim(app->actors[id], TRUE);
}

BOOL Box2Obj_IsAnimActive(Box2AppWork *app, u32 id) {
    return ClActor_IsAnimActive(app->actors[id]);
}

void Box2Obj_SetVisible(Box2AppWork *app, u32 id, BOOL visible) {
    if (app->actors[id] != NULL) {
        ClActor_SetVisible(app->actors[id], visible);
    }
}

BOOL Box2Obj_IsVisible(Box2AppWork *app, u32 id) {
    return ClActor_IsVisible(app->actors[id]);
}

static void SetBgPriority(Box2AppWork *app, u32 id, u32 priority) {
    ClActor_SetBgPriority(app->actors[id], (u8)priority);
}

static void SetSoftPriority(Box2AppWork *app, u32 id, u32 priority) {
    ClActor_SetPriority(app->actors[id], priority);
}

static void SetBlend(Box2AppWork *app, u32 id, BOOL blend) {
    if (blend == TRUE) {
        ClActor_SetObjMode(app->actors[id], GX_OAM_MODE_XLU);
    } else {
        ClActor_SetObjMode(app->actors[id], GX_OAM_MODE_NORMAL);
    }
}

void Box2Obj_SetPos(Box2AppWork *app, u32 id, s16 x, s16 y, u16 surface) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    ClActor_SetSurfacePos(app->actors[id], &pos, surface);
}

void Box2Obj_GetPos(Box2AppWork *app, u32 id, s16 *x, s16 *y, u16 surface) {
    ClActorPos pos;

    ClActor_GetSurfacePos(app->actors[id], &pos, surface);
    *x = pos.x;
    *y = pos.y;
}

static void SetPalette(Box2AppWork *app, u32 id, u32 palette) {
    ClActor_SetPalette(app->actors[id], (u8)palette, 1);
}

static void LoadBoxResources(Box2AppWork *app) {
    ArcTool *arc;
    u32 i;
    u32 j;

    arc = GFL_ArcSysCreateFileHandle(ARCID_BOX2, HEAPID_BOX2_APP);
    for (i = 0; i < 6; i++) {
        app->chrRes[BOX2_CHAR_TRAY_ICON_FIRST + i] =
            ClActSys_LoadChars(arc, 71, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    }
    app->palRes[BOX2_PAL_TRAY_ICON] = ClActSys_LoadPltt(arc, 72, CLACT_VRAM_MAIN, 0, HEAPID_BOX2_APP);
    app->cellRes[BOX2_CELL_TRAY_ICON] = ClActSys_LoadCellAnims(arc, 73, 74, HEAPID_BOX2_APP);
    for (j = 0; j < 96; j++) {
        app->chrRes[j] = ClActSys_LoadChars(arc, 75, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    }
    app->chrRes[BOX2_CHAR_BOX_UI] = ClActSys_LoadChars(arc, 67, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->palRes[BOX2_PAL_BOX_UI] = ClActSys_LoadPltt(arc, 70, CLACT_VRAM_MAIN, 0xe0, HEAPID_BOX2_APP);
    app->cellRes[BOX2_CELL_BOX_UI] = ClActSys_LoadCellAnims(arc, 68, 69, HEAPID_BOX2_APP);
    app->chrRes[BOX2_CHAR_SUMMARY_BUTTON] = ClActSys_LoadChars(arc, 64, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->cellRes[BOX2_CELL_SUMMARY_BUTTON] = ClActSys_LoadCellAnims(arc, 65, 66, HEAPID_BOX2_APP);
    app->chrRes[BOX2_CHAR_ITEM_ICON] = ClActSys_LoadChars(arc, 76, TRUE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->cellRes[BOX2_CELL_ITEM_ICON] = ClActSys_LoadCellAnims(arc, 77, 78, HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

static void LoadUiResources(Box2AppWork *app) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_BOX2_APP);

    app->chrRes[BOX2_CHAR_TOUCH_BAR] =
        ClActSys_LoadChars(arc, AppMenuCommon_GetTouchBarCharFile(), FALSE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->palRes[BOX2_PAL_TOUCH_BAR] =
        ClActSys_LoadPltt(arc, AppMenuCommon_GetTouchBarPaletteFile(), CLACT_VRAM_MAIN, 0x120, HEAPID_BOX2_APP);
    app->cellRes[BOX2_CELL_TOUCH_BAR] =
        ClActSys_LoadCellAnims(arc, AppMenuCommon_GetTouchBarCellFile(BOX2_OBJ_MAPPING_128K),
                               AppMenuCommon_GetTouchBarAnimFile(BOX2_OBJ_MAPPING_128K), HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

static void LoadPokeIconResources(Box2AppWork *app) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_BOX2_APP);

    app->palRes[BOX2_PAL_POKE_ICON] =
        ClActSys_LoadPlttEx(arc, PokeIcon_GetAltPaletteFile(), CLACT_VRAM_MAIN, 0x60, HEAPID_BOX2_APP);
    app->cellRes[BOX2_CELL_POKE_ICON] =
        ClActSys_LoadCellAnims(arc, PokeIcon_GetCellFile_Main(), getOBJTileMapping_MainEng(), HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

static void CreatePokeIconActors(Box2AppWork *app) {
    Box2ActorData data;
    Box2ActorData base = sPokeIconData;
    u32 i;

    for (i = 0; i < NELEMS(app->pokeIconId); i++) {
        data = base;

        data.setup.x += (s16)(i % 6 * 24);
        data.setup.y += (s16)(i / 6 * 24);
        data.setup.priority = 146 - i * 2;
        data.chr = i;
        app->actors[BOX2_ACTOR_POKEICON + i] = CreateActor(app, &data);
        app->pokeIconId[i] = BOX2_ACTOR_POKEICON + i;
        Box2Obj_SetVisible(app, BOX2_ACTOR_POKEICON + i, FALSE);
    }
}

static void *ReadPokeIconChar(Box2AppWork *app, BoxPkm *pkm, NNSG2dCharacterData **chr) {
    return GFL_G2DIOReadOBJNCGRArc(app->pokeIconArc, PokeIcon_GetFileOfPkm(pkm), FALSE, chr, HEAPID_BOX2_APP);
}

void Box2Obj_LoadTrayPokeIcons(Box2SysWork *syswk, u32 tray) {
    BoxPkm *pkm;
    void *buf;
    NNSG2dCharacterData *chr;
    u32 i;

    for (i = 0; i < BOX2_TRAY_POKE_MAX; i++) {
        if (BoxSaveAccessor_GetPkmParam(syswk->param->boxes, tray, i, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            syswk->app->pokeIconExist[i] = FALSE;
        } else if (syswk->param->mode != BOX2_MODE_MOVE_ITEMS && syswk->tray == syswk->getTray &&
                   IsInPickedRange(syswk, i) == TRUE) {
            syswk->app->pokeIconExist[i] = FALSE;
        } else {
            pkm = Box2Main_GetBoxPkm(syswk, tray, i);
            buf = ReadPokeIconChar(syswk->app, pkm, &chr);
            MIi_CpuCopy32(chr->pRawData, syswk->app->pokeIconChar[i], 0x200);
            GFL_HeapFree(buf);
            syswk->app->pokeIconPal[i] = GetPokeIconPalette(syswk, pkm);
            syswk->app->pokeIconExist[i] = TRUE;
        }
    }
}

static BOOL MatchesSearch(Box2SearchParam *search, BoxPkm *pkm) {
    BOOL egg;
    u32 marks;
    u32 i;

    if (search->active == FALSE) {
        return TRUE;
    }
    egg = PML_PkmGetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    if (search->species != 0) {
        if (search->species != PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL) || egg != FALSE) {
            return FALSE;
        }
    }
    if (search->item == BOX2_SEARCH_ITEM_WITHOUT) {
        if (PML_PkmGetParam(pkm, PKM_PARAM_ITEM, NULL) != 0 || egg != FALSE) {
            return FALSE;
        }
    } else if (search->item == BOX2_SEARCH_ITEM_WITH) {
        if (PML_PkmGetParam(pkm, PKM_PARAM_ITEM, NULL) == 0 || egg != FALSE) {
            return FALSE;
        }
    }
    if (search->nature != 0) {
        if (search->nature - 1 != PML_PkmGetParam(pkm, PKM_PARAM_NATURE, NULL) || egg != FALSE) {
            return FALSE;
        }
    }
    if (search->ability != 0) {
        if (search->ability != PML_PkmGetParam(pkm, PKM_PARAM_ABILITY, NULL) || egg != FALSE) {
            return FALSE;
        }
    }
    if (search->sex != BOX2_SEARCH_GENDER_ANY) {
        if (search->sex - 1 != PML_PkmGetParam(pkm, PKM_PARAM_SEX, NULL) || egg != FALSE) {
            return FALSE;
        }
    }
    if (search->marks != 0) {
        marks = PML_PkmGetParam(pkm, PKM_PARAM_MARKINGS, NULL);
        for (i = 0; i < BOX2_MARK_COUNT; i++) {
            if ((search->marks & (1 << i)) && !(marks & (1 << i))) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static u32 GetPokeIconPalette(Box2SysWork *syswk, BoxPkm *pkm) {
    if (MatchesSearch(&syswk->search, pkm)) {
        return PokeIcon_GetPaletteOfPkm(pkm);
    }
    return 3;
}

static void TransferIconChar(Box2AppWork *app, void *chr, u32 id, u32 palette) {
    ClActSys_WriteCharData(ClActor_GetCharIdx(app->actors[id]), chr, 0x200, 0, CLACT_VRAM_MAIN);
    ClActor_SetPalette(app->actors[id], (u8)palette, 1);
}

static void SetPokeIcon(Box2SysWork *syswk, BoxPkm *pkm, u32 id) {
    NNSG2dCharacterData *chr;
    void *buf = ReadPokeIconChar(syswk->app, pkm, &chr);

    TransferIconChar(syswk->app, chr->pRawData, id, GetPokeIconPalette(syswk, pkm));
    GFL_HeapFree(buf);
}

void Box2Obj_SetTrayPokeIcon(Box2SysWork *syswk, u32 tray, u32 pos, u32 id) {
    Box2AppWork *app = syswk->app;

    Box2Obj_SetVisible(app, id, FALSE);
    if (Box2Main_GetPokeParam(syswk, pos, tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
        SetPokeIcon(syswk, Box2Main_GetBoxPkm(syswk, tray, pos), id);
        Box2Obj_SetVisible(app, id, TRUE);
    }
}

void Box2Obj_SetTrayPokeIcons(Box2SysWork *syswk) {
    u32 i;

    for (i = 0; i < BOX2_TRAY_POKE_MAX; i++) {
        Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, i, BOX2_ACTOR_POKEICON + i);
    }
}

void Box2Obj_SetPartyPokeIcons(Box2SysWork *syswk) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < count; i++) {
        SetPokeIcon(syswk, PokeParty_ToBoxPkm(PokeParty_GetPkm(syswk->param->party, i)),
                    syswk->app->pokeIconId[BOX2_PARTY_POS + i]);
        Box2Obj_SetVisible(syswk->app, syswk->app->pokeIconId[BOX2_PARTY_POS + i], FALSE);
    }
}

void Box2Obj_GetIconPos(u32 pos, s16 *x, s16 *y, u32 mode) {
    if (pos < BOX2_TRAY_POKE_MAX) {
        *x = (pos % 6 + 1) * 24;
        *y = pos / 6 * 24 + 48;
    } else {
        pos -= BOX2_TRAY_POKE_MAX;
        *x = sPartyPokePos[pos].x;
        *y = sPartyPokePos[pos].y - 144;
        if (mode == BOX2_ICON_POS_PARTY_RIGHT) {
            *x += 152;
        }
    }
}

void Box2Obj_PutPartyIconsLeftBelow(Box2SysWork *syswk, BOOL hideGet) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < PARTY_SIZE; i++) {
        u32 id = syswk->app->pokeIconId[BOX2_PARTY_POS + i];

        Box2Obj_SetPos(syswk->app, id, sPartyPokePos[i].x, sPartyPokePos[i].y, CLACT_SURFACE_MAIN);
        Box2Obj_SetIconPriority(syswk->app, BOX2_PARTY_POS + i, TRUE);
        if (i < count && (hideGet == FALSE || syswk->pos != BOX2_PARTY_POS + i)) {
            Box2Obj_SetVisible(syswk->app, id, TRUE);
        } else {
            Box2Obj_SetVisible(syswk->app, id, FALSE);
        }
    }
}

void Box2Obj_HidePartyRangeIcons(Box2SysWork *syswk) {
    u32 i;
    u32 pos;
    u32 x;

    if (syswk->pos >= BOX2_PARTY_POS) {
        pos = syswk->pos - BOX2_PARTY_POS;
        x = pos & 1;
        for (i = 0; i < PARTY_SIZE; i++) {
            if ((i & 1) >= x && (i & 1) < x + syswk->app->rangeWidth && (i >> 1) >= (pos >> 1) &&
                (i >> 1) < (pos >> 1) + syswk->app->rangeHeight) {
                Box2Obj_SetVisible(syswk->app, syswk->app->pokeIconId[BOX2_PARTY_POS + i], FALSE);
            }
        }
    }
}

void Box2Obj_PutPartyIconsRightBelow(Box2SysWork *syswk) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < PARTY_SIZE; i++) {
        u32 id = syswk->app->pokeIconId[BOX2_PARTY_POS + i];

        Box2Obj_SetPos(syswk->app, id, sPartyPokePos[i].x + 152, sPartyPokePos[i].y, CLACT_SURFACE_MAIN);
        Box2Obj_SetIconPriority(syswk->app, BOX2_PARTY_POS + i, TRUE);
        if (i < count) {
            Box2Obj_SetVisible(syswk->app, id, TRUE);
        } else {
            Box2Obj_SetVisible(syswk->app, id, FALSE);
        }
    }
}

void Box2Obj_PutPartyIconsLeft(Box2SysWork *syswk) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < PARTY_SIZE; i++) {
        u32 id = syswk->app->pokeIconId[BOX2_PARTY_POS + i];

        Box2Obj_SetPos(syswk->app, id, sPartyPokePos[i].x, sPartyPokePos[i].y - 144, CLACT_SURFACE_MAIN);
        Box2Obj_SetIconPriority(syswk->app, BOX2_PARTY_POS + i, TRUE);
        if (i < count) {
            Box2Obj_SetVisible(syswk->app, id, TRUE);
        } else {
            Box2Obj_SetVisible(syswk->app, id, FALSE);
        }
    }
}

void Box2Obj_PutPartyIconsRight(Box2SysWork *syswk) {
    u32 count = PokeParty_GetPkmCount(syswk->param->party);
    u32 i;

    for (i = 0; i < PARTY_SIZE; i++) {
        u32 id = syswk->app->pokeIconId[BOX2_PARTY_POS + i];

        Box2Obj_SetPos(syswk->app, id, sPartyPokePos[i].x + 152, sPartyPokePos[i].y - 144, CLACT_SURFACE_MAIN);
        Box2Obj_SetIconPriority(syswk->app, BOX2_PARTY_POS + i, TRUE);
        if (i < count) {
            Box2Obj_SetVisible(syswk->app, id, TRUE);
        } else {
            Box2Obj_SetVisible(syswk->app, id, FALSE);
        }
    }
}

void Box2Obj_SetIconPriority(Box2AppWork *app, u32 iconPos, BOOL put) {
    u32 id = app->pokeIconId[iconPos];

    if (put == FALSE) {
        SetBgPriority(app, id, 0);
        SetSoftPriority(app, id, 20);
    } else {
        if (iconPos < BOX2_TRAY_POKE_MAX || iconPos == BOX2_BOXLIST_POS) {
            SetBgPriority(app, id, 3);
        } else {
            SetBgPriority(app, id, 1);
        }
        SetSoftPriority(app, id, 146 - iconPos * 2);
    }
}

void Box2Obj_SetIconPriorityAt(Box2AppWork *app, u32 iconPos, u32 pos, BOOL put) {
    u32 id = app->pokeIconId[iconPos];

    if (put == FALSE) {
        SetBgPriority(app, id, 0);
        SetSoftPriority(app, id, 20);
    } else {
        if (pos < BOX2_TRAY_POKE_MAX) {
            SetBgPriority(app, id, 3);
        } else {
            SetBgPriority(app, id, 1);
        }
        SetSoftPriority(app, id, 146 - pos * 2);
    }
}

void Box2Obj_ScrollTrayIcons(Box2SysWork *syswk, s32 mv) {
    s16 start;
    int end;
    s16 x;
    s16 y;
    u16 i;
    u32 id;

    if (mv >= 0) {
        end = 176;
        start = -8;
    } else {
        end = -8;
        start = 176;
    }
    for (i = 0; i < BOX2_TRAY_POKE_MAX; i++) {
        id = syswk->app->pokeIconId[i];
        Box2Obj_GetPos(syswk->app, id, &x, &y, CLACT_SURFACE_MAIN);
        x = x + mv;
        if (x == end) {
            x = start;
            TransferIconChar(syswk->app, syswk->app->pokeIconChar[i], id, syswk->app->pokeIconPal[i]);
            Box2Obj_SetVisible(syswk->app, id, syswk->app->pokeIconExist[i]);
            if (syswk->app->boxJumpOpen == 0 &&
                (syswk->param->mode == BOX2_MODE_MOVE_ITEMS || syswk->param->mode == BOX2_MODE_DREAM_WORLD)) {
                SetIconBlendChecked(syswk, i);
            }
        }
        Box2Obj_SetPos(syswk->app, id, x, y, CLACT_SURFACE_MAIN);
    }
}

void Box2Obj_MovePartyIconsWithFrame(Box2SysWork *syswk) {
    s8 x;
    s8 y;
    s16 px;
    s16 py;
    u16 i;

    BGWinFrame_GetPos(syswk->app->bgWinFrame, BOX2_FRAME_PARTY, &x, &y);
    py = y * 8;
    px = x * 8;
    for (i = 0; i < PARTY_SIZE; i++) {
        Box2Obj_SetPos(syswk->app, syswk->app->pokeIconId[BOX2_PARTY_POS + i], px + sPartyFramePokePos[i][0],
                       py + sPartyFramePokePos[i][1], CLACT_SURFACE_MAIN);
    }
}

void Box2PokeFree_Start(Box2PokeFreeWork *wk) {
    ClActorScale scale;
    ClActorPos center;

    wk->scale = 4096.0f;
    wk->scaleCnt = 0;
    ClActor_SetAffineMode(wk->cap, NNS_G2D_RND_AFFINE_OVERWRITE_NORMAL);
    scale.x = wk->scale;
    scale.y = wk->scale;
    ClActor_SetScale(wk->cap, &scale);
    center.x = 0;
    center.y = 8;
    ClActor_SetAffineCenter(wk->cap, &center);
}

BOOL Box2PokeFree_Shrink(Box2PokeFreeWork *wk) {
    ClActorScale scale;

    wk->scaleCnt++;
    wk->scale -= 102.0f;
    if (wk->scaleCnt == 40) {
        return FALSE;
    }
    scale.x = wk->scale;
    scale.y = wk->scale;
    ClActor_SetScale(wk->cap, &scale);
    return TRUE;
}

BOOL Box2PokeFree_Grow(Box2PokeFreeWork *wk) {
    ClActorScale scale;

    wk->scaleCnt -= 2;
    wk->scale += 102.0f;
    wk->scale += 102.0f;
    if (wk->scaleCnt == 0) {
        return FALSE;
    }
    scale.x = wk->scale;
    scale.y = wk->scale;
    ClActor_SetScale(wk->cap, &scale);
    return TRUE;
}

void Box2PokeFree_End(Box2PokeFreeWork *wk) {
    ClActor_SetVisible(wk->cap, FALSE);
    Box2PokeFree_ResetActor(wk);
}

void Box2PokeFree_ResetActor(Box2PokeFreeWork *wk) {
    ClActorScale scale;
    ClActorPos center;

    center.x = 0;
    center.y = 0;
    ClActor_SetAffineCenter(wk->cap, &center);
    scale.x = FX32_ONE;
    scale.y = FX32_ONE;
    ClActor_SetScale(wk->cap, &scale);
    ClActor_SetAffineMode(wk->cap, NNS_G2D_RND_AFFINE_OVERWRITE_NONE);
}

static void SetIconsBlend(Box2SysWork *syswk, BOOL blend, u32 start, u32 end, BOOL check) {
    u32 i;

    if (check == TRUE) {
        if (syswk->param->mode == BOX2_MODE_DREAM_WORLD) {
            for (i = start; i < end; i++) {
                if (Box2Main_IsSpeciesSendable(syswk, i) == TRUE &&
                    Box2Main_GetPokeParam(syswk, i, syswk->tray, PKM_PARAM_IS_EGG, NULL) == 0) {
                    SetBlend(syswk->app, syswk->app->pokeIconId[i], FALSE);
                } else {
                    SetBlend(syswk->app, syswk->app->pokeIconId[i], TRUE);
                }
            }
        } else {
            for (i = start; i < end; i++) {
                if (Box2Main_GetPokeParam(syswk, i, syswk->tray, PKM_PARAM_ITEM, NULL) == 0) {
                    SetBlend(syswk->app, syswk->app->pokeIconId[i], TRUE);
                } else {
                    SetBlend(syswk->app, syswk->app->pokeIconId[i], FALSE);
                }
            }
        }
    } else {
        for (i = start; i < end; i++) {
            SetBlend(syswk->app, syswk->app->pokeIconId[i], blend);
        }
    }
}

void Box2Obj_SetIconsBlend(Box2SysWork *syswk, u32 flags, BOOL blend) {
    BOOL check;

    if (flags & BOX2_ICON_CHECK_ELIGIBILITY) {
        check = TRUE;
    } else {
        check = FALSE;
    }
    if (flags & BOX2_ICON_GROUP_TRAY) {
        SetIconsBlend(syswk, blend, 0, BOX2_TRAY_POKE_MAX, check);
    }
    if (flags & BOX2_ICON_GROUP_PARTY_AND_HELD) {
        SetIconsBlend(syswk, blend, BOX2_TRAY_POKE_MAX, NELEMS(syswk->app->pokeIconId), check);
    }
}

void Box2Obj_SetIconBlend(Box2AppWork *app, u32 iconPos, BOOL blend) {
    SetBlend(app, app->pokeIconId[iconPos], blend);
}

static void SetIconBlendChecked(Box2SysWork *syswk, u32 pos) {
    SetIconsBlend(syswk, TRUE, pos, pos + 1, TRUE);
}

void Box2Obj_PickUpRangeIcons(Box2SysWork *syswk, u32 pos, int width, int height) {
    int i;
    int j;
    Box2AppWork *app;
    unsigned int y;
    u32 x;
    unsigned int rowWidth;
    s16 px;
    s16 py;
    u8 tmp;

    app = syswk->app;
    rowWidth = Box2Main_GetRowWidth(syswk, pos);
    x = pos % rowWidth;
    y = pos / rowWidth;
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            Box2Obj_GetPos(app, app->pokeIconId[x + (y + j) * rowWidth + i], &px, &py, CLACT_SURFACE_MAIN);
            tmp = app->pokeIconId[BOX2_BOXLIST_POS + j * width + i];
            app->pokeIconId[BOX2_BOXLIST_POS + j * width + i] = app->pokeIconId[x + (y + j) * rowWidth + i];
            app->pokeIconId[x + (y + j) * rowWidth + i] = tmp;
            Box2Obj_SetPos(app, app->pokeIconId[x + (y + j) * rowWidth + i], px, py, CLACT_SURFACE_MAIN);
            Box2Obj_SetIconPriority(app, x + (y + j) * rowWidth + i, TRUE);
        }
    }
    app->rangeWidth = width;
    app->rangeHeight = height;
}

void Box2Obj_PutDownRangeIcons(Box2AppWork *app, u32 pos, int width, int height) {
    s16 px;
    s16 py;
    u32 x;
    unsigned int y;
    int i;
    int j;
    u8 tmp;

    Box2Obj_GetPos(app, app->pokeIconId[BOX2_BOXLIST_POS], &px, &py, CLACT_SURFACE_MAIN);
    x = pos % 6;
    y = pos / 6;
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            Box2Obj_GetPos(app, app->pokeIconId[BOX2_BOXLIST_POS + j * 6 + i], &px, &py, CLACT_SURFACE_MAIN);
            tmp = app->pokeIconId[x + (y + j) * 6 + i];
            app->pokeIconId[x + (y + j) * 6 + i] = app->pokeIconId[BOX2_BOXLIST_POS + j * 6 + i];
            app->pokeIconId[BOX2_BOXLIST_POS + j * 6 + i] = tmp;

            Box2Obj_SetPos(app, app->pokeIconId[BOX2_BOXLIST_POS + j * 6 + i], px, py, CLACT_SURFACE_MAIN);
        }
    }
}

void Box2Obj_MoveHeldIcons(Box2SysWork *syswk) {
    s16 x;
    s16 y;
    int j;
    int i;

    if (syswk->pickState != BOX2_PICK_NONE) {
        Box2Obj_GetPos(syswk->app, BOX2_ACTOR_CURSOR, &x, &y, CLACT_SURFACE_MAIN);
        if (syswk->param->mode == BOX2_MODE_MOVE_ITEMS) {
            y += 8;
            Box2Obj_SetPos(syswk->app, BOX2_ACTOR_ITEM_ICON, x, y, CLACT_SURFACE_MAIN);
        } else {
            y += 4;
            for (j = 0; j < syswk->app->rangeHeight; j++) {
                for (i = 0; i < syswk->app->rangeWidth; i++) {
                    Box2Obj_SetPos(syswk->app,
                                   syswk->app->pokeIconId[BOX2_BOXLIST_POS + j * syswk->app->rangeWidth + i],
                                   x + i * 24, y + j * 24, CLACT_SURFACE_MAIN);
                }
            }
            Box2Obj_PutRangeCount(syswk->app, x, y + 9);
        }
        if (syswk->param->mode == BOX2_MODE_MOVE_ITEMS) {
            Box2Obj_MoveItemOutline(syswk->app);
        } else {
            if (syswk->app->actors[BOX2_ACTOR_ITEM_OUTLINE] == NULL) {
                Box2Obj_PutPokeOutline(syswk);
            }
            if (syswk->moveMode != BOX2_MOVE_MODE_RANGE) {
                Box2Obj_MoveOutlineToIcon(syswk->app, BOX2_BOXLIST_POS);
            } else {
                Box2Obj_ShowPokeOutline(syswk, FALSE);
            }
        }
    }
}

void Box2Obj_UpdatePokeIconPalette(Box2SysWork *syswk, u32 tray, u32 pos) {
    u32 palette = GetPokeIconPalette(syswk, Box2Main_GetBoxPkm(syswk, tray, pos));

    if (pos < BOX2_TRAY_POKE_MAX) {
        syswk->app->pokeIconPal[pos] = palette;
    }
    SetPalette(syswk->app, syswk->app->pokeIconId[pos], palette);
}

static void ClearPokeGraResources(Box2AppWork *app) {
    app->chrRes[BOX2_CHAR_POKE_GRA_FIRST] = BOX2_RES_NONE;
    app->palRes[BOX2_PAL_POKE_GRA_FIRST] = BOX2_RES_NONE;
    app->cellRes[BOX2_CELL_POKE_GRA_FIRST] = BOX2_RES_NONE;
    app->chrRes[BOX2_CHAR_POKE_GRA_SECOND] = BOX2_RES_NONE;
    app->palRes[BOX2_PAL_POKE_GRA_SECOND] = BOX2_RES_NONE;
    app->cellRes[BOX2_CELL_POKE_GRA_SECOND] = BOX2_RES_NONE;
}

void Box2Obj_ShowPokeGra(Box2SysWork *syswk, Box2PokeInfo *info, u32 id) {
    u32 *chr;
    u32 *pal;
    u32 *cell;
    u32 other;
    u16 palOffset;
    ArcTool *arc;
    BOOL encrypted;

    if (syswk->app->pokegraSwap == 0) {
        chr = &syswk->app->chrRes[BOX2_CHAR_POKE_GRA_FIRST];
        pal = &syswk->app->palRes[BOX2_PAL_POKE_GRA_FIRST];
        cell = &syswk->app->cellRes[BOX2_CELL_POKE_GRA_FIRST];
        palOffset = 0x60;
        other = id + 1;
    } else {
        chr = &syswk->app->chrRes[BOX2_CHAR_POKE_GRA_SECOND];
        pal = &syswk->app->palRes[BOX2_PAL_POKE_GRA_SECOND];
        cell = &syswk->app->cellRes[BOX2_CELL_POKE_GRA_SECOND];
        palOffset = 0x80;
        other = id;
        id++;
    }
    if (syswk->app->actors[id] != NULL) {
        ClActor_Delete(syswk->app->actors[id]);
        ClActSys_FreeChars(*chr);
        ClActSys_FreePltt(*pal);
        ClActSys_FreeCellAnims(*cell);
    }
    arc = MakePokeGraArcHandle(HEAPID_BOX2_APP);
    encrypted = PML_PkmDecrypt(info->pkm);
    *chr = PokeGra_LoadClActCharsByBoxData(arc, info->pkm, POKEGRA_DIR_FRONT, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    *pal = PokeGra_LoadClActPaletteByBoxData(arc, info->pkm, POKEGRA_DIR_FRONT, CLACT_VRAM_SUB, palOffset,
                                             HEAPID_BOX2_APP);
    *cell = PokeGra_LoadClActCellAnimsByBoxData(info->pkm, POKEGRA_DIR_FRONT, 2, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    PML_PkmReEncrypt(info->pkm, encrypted);
    GFL_ArcToolFree(arc);
    syswk->app->actors[id] = CreateActor(syswk->app, &sActorData[id]);
    Box2Obj_SetVisible(syswk->app, id, TRUE);
    if (syswk->app->actors[other] != NULL) {
        Box2Obj_SetVisible(syswk->app, other, FALSE);
    }
    syswk->app->pokegraSwap ^= 1;
}

static void LoadInfoResources(Box2AppWork *app) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_BOX2_APP | HEAPID_TAIL_BIT);
    u32 i;

    for (i = 0; i < 17; i++) {
        app->chrRes[BOX2_CHAR_TYPE_ICON_FIRST + i] =
            ClActSys_LoadChars(arc, AppMenuCommon_GetTypeIconCharFile(i), FALSE, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    }
    app->cellRes[BOX2_CELL_TYPE_ICON] =
        ClActSys_LoadCellAnims(arc, AppMenuCommon_GetTypeIconCellFile(BOX2_OBJ_MAPPING_128K),
                               AppMenuCommon_GetTypeIconAnimFile(BOX2_OBJ_MAPPING_128K), HEAPID_BOX2_APP);
    app->palRes[BOX2_PAL_TYPE_ICON] =
        ClActSys_LoadPltt(arc, AppMenuCommon_GetTypeIconPaletteFile(), CLACT_VRAM_SUB, 0, HEAPID_BOX2_APP);
    app->chrRes[BOX2_CHAR_MARK_SUB] = ClActSys_LoadChars(
        arc, AppMenuCommon_GetMarkingIconCharFile(BOX2_OBJ_MAPPING_128K), FALSE, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    app->cellRes[BOX2_CELL_MARK_SUB] =
        ClActSys_LoadCellAnims(arc, AppMenuCommon_GetMarkingIconCellFile(BOX2_OBJ_MAPPING_128K),
                               AppMenuCommon_GetMarkingIconAnimFile(BOX2_OBJ_MAPPING_128K), HEAPID_BOX2_APP);
    app->palRes[BOX2_PAL_MARK_SUB] =
        ClActSys_LoadPltt(arc, AppMenuCommon_GetMarkingIconPaletteFile(), CLACT_VRAM_SUB, 0xa0, HEAPID_BOX2_APP);
    app->chrRes[BOX2_CHAR_POKERUS] = ClActSys_LoadChars(
        arc, AppMenuCommon_GetPokerusIconCharFile(BOX2_OBJ_MAPPING_128K), FALSE, CLACT_VRAM_SUB, HEAPID_BOX2_APP);
    app->cellRes[BOX2_CELL_POKERUS] =
        ClActSys_LoadCellAnims(arc, AppMenuCommon_GetPokerusIconCellFile(BOX2_OBJ_MAPPING_128K),
                               AppMenuCommon_GetPokerusIconAnimFile(BOX2_OBJ_MAPPING_128K), HEAPID_BOX2_APP);
    app->palRes[BOX2_PAL_POKERUS] =
        ClActSys_LoadPltt(arc, AppMenuCommon_GetPokerusIconPaletteFile(), CLACT_VRAM_SUB, 0xc0, HEAPID_BOX2_APP);
    app->chrRes[BOX2_CHAR_MARK_MAIN] = ClActSys_LoadChars(
        arc, AppMenuCommon_GetMarkingIconCharFile(BOX2_OBJ_MAPPING_128K), FALSE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->cellRes[BOX2_CELL_MARK_MAIN] =
        ClActSys_LoadCellAnims(arc, AppMenuCommon_GetMarkingIconCellFile(BOX2_OBJ_MAPPING_128K),
                               AppMenuCommon_GetMarkingIconAnimFile(BOX2_OBJ_MAPPING_128K), HEAPID_BOX2_APP);
    app->palRes[BOX2_PAL_MARK_MAIN] =
        ClActSys_LoadPltt(arc, AppMenuCommon_GetMarkingIconPaletteFile(), CLACT_VRAM_MAIN, 0x1c0, HEAPID_BOX2_APP);
    app->chrRes[BOX2_CHAR_SEARCH_BUTTON] = ClActSys_LoadChars(arc, 175, FALSE, CLACT_VRAM_MAIN, HEAPID_BOX2_APP);
    app->cellRes[BOX2_CELL_SEARCH_BUTTON] = ClActSys_LoadCellAnims(arc, 174, 173, HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

static void CreateTypeIconActors(Box2AppWork *app) {
    Box2ActorData data = sTypeIconData;
    u32 i;

    for (i = 0; i < 17; i++) {
        data.chr = BOX2_CHAR_TYPE_ICON_FIRST + i;
        app->actors[BOX2_ACTOR_TYPE_ICON + i] = CreateActor(app, &data);
        ClActor_SetPalette(app->actors[BOX2_ACTOR_TYPE_ICON + i], AppMenuCommon_GetTypeIconPaletteIndex(i), 1);
        Box2Obj_SetVisible(app, 32 + i, FALSE);
    }
}

static void ShowTypeIcons(Box2AppWork *app, u32 type1, u32 type2) {
    u32 id = BOX2_ACTOR_TYPE_ICON + type1;

    Box2Obj_SetVisible(app, id, TRUE);
    Box2Obj_SetPos(app, id, 88, 56, CLACT_SURFACE_SUB);
    if (type2 != 0 && type1 != type2) {
        id = BOX2_ACTOR_TYPE_ICON + type2;
        Box2Obj_SetVisible(app, id, TRUE);
        Box2Obj_SetPos(app, id, 122, 56, CLACT_SURFACE_SUB);
    }
}

void Box2Obj_PutTypeIcons(Box2AppWork *app, Box2PokeInfo *info) {
    u32 i;

    for (i = BOX2_ACTOR_TYPE_ICON; i < BOX2_ACTOR_TYPE_ICON + 17; i++) {
        Box2Obj_SetVisible(app, i, FALSE);
    }
    if (info->egg == 0) {
        ShowTypeIcons(app, info->type1, info->type2);
    }
}

static void LoadItemIconPalette(Box2AppWork *app) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ITEMGRA, HEAPID_BOX2_APP);

    app->palRes[BOX2_PAL_ITEM_ICON] =
        ClActSys_LoadPltt(arc, GetItemGraphicsDatID(0, ITEM_FILE_ICON_PLTT), CLACT_VRAM_MAIN, 0x1a0, HEAPID_BOX2_APP);
    GFL_ArcToolFree(arc);
}

void Box2Obj_SetItemIcon(Box2AppWork *app, u16 item) {
    NNSG2dCharacterData *chr;
    NNSG2dPaletteData *pal;
    void *buf;

    buf = GFL_G2DIOReadOBJNCGR(PML_ItemGetIconArcID(), GetItemGraphicsDatID(item, ITEM_FILE_ICON_CHAR), FALSE, &chr,
                               HEAPID_BOX2_APP);
    ClActSys_ReplaceCharData(ClActor_GetCharIdx(app->actors[BOX2_ACTOR_ITEM_ICON]), chr);
    GFL_HeapFree(buf);
    buf = GFL_G2DIOReadNCLR(PML_ItemGetIconArcID(), GetItemGraphicsDatID(item, ITEM_FILE_ICON_PLTT), &pal,
                            HEAPID_BOX2_APP);
    ClActSys_ReplacePlttData(ClActor_GetPlttIdx(app->actors[BOX2_ACTOR_ITEM_ICON]), pal, 1);
    GFL_HeapFree(buf);
}

void Box2Obj_SetItemIconAffine(Box2AppWork *app, BOOL affine) {
    ClActorPos center;

    if (affine == TRUE) {
        ClActor_SetAffineMode(app->actors[BOX2_ACTOR_ITEM_ICON], NNS_G2D_RND_AFFINE_OVERWRITE_NORMAL);
        center.x = 12;
        center.y = 12;
        ClActor_SetAffineCenter(app->actors[BOX2_ACTOR_ITEM_ICON], &center);
    } else {
        center.x = 0;
        center.y = 0;
        ClActor_SetAffineCenter(app->actors[BOX2_ACTOR_ITEM_ICON], &center);
        ClActor_SetAffineMode(app->actors[BOX2_ACTOR_ITEM_ICON], NNS_G2D_RND_AFFINE_OVERWRITE_NONE);
    }
}

void Box2Obj_SetItemIconPos(Box2AppWork *app, s16 x, s16 y) {
    Box2Obj_SetPos(app, BOX2_ACTOR_ITEM_ICON, x, y, CLACT_SURFACE_MAIN);
}

void Box2Obj_PutItemIconAtIcon(Box2AppWork *app, u32 pos, u32 mode) {
    s16 x;
    s16 y;

    Box2Obj_GetIconPos(pos, &x, &y, mode);
    Box2Obj_SetItemIconPos(app, x + 8, y + 8);
}

void Box2Obj_PutItemIconAtCursor(Box2AppWork *app, u32 pos, u32 mode) {
    s16 x;
    s16 y;

    Box2Obj_GetIconPos(pos, &x, &y, mode);
    Box2Obj_SetItemIconPos(app, x, y + 4);
}

void Box2Obj_InitItemOutline(Box2AppWork *app) {
    Box2ActorData data;
    s16 x;
    s16 y;
    u32 i;

    if (app->getItem != 0) {
        Box2Obj_GetPos(app, BOX2_ACTOR_ITEM_ICON, &x, &y, CLACT_SURFACE_MAIN);
        if (app->actors[BOX2_ACTOR_ITEM_OUTLINE] == NULL) {
            data = sActorData[BOX2_ACTOR_ITEM_ICON];
            data.setup.priority = ClActor_GetPriority(app->actors[BOX2_ACTOR_ITEM_ICON]) + 1;
            data.setup.bgPriority = ClActor_GetBgPriority(app->actors[BOX2_ACTOR_ITEM_ICON]);
            data.chr = BOX2_CHAR_ITEM_ICON;
            data.pal = BOX2_PAL_BOX_UI;
            data.palette = 1;
            for (i = 0; i < 8; i++) {
                data.setup.x = x + sItemOutlineX[i];
                data.setup.y = y + sItemOutlineY[i];
                app->actors[BOX2_ACTOR_ITEM_OUTLINE + i] = CreateActor(app, &data);
                ClActor_SetPalette(app->actors[BOX2_ACTOR_ITEM_OUTLINE + i], (u8)data.palette, 1);
                Box2Obj_SetVisible(app, BOX2_ACTOR_ITEM_OUTLINE + i, FALSE);
            }
        } else {
            u32 priority = ClActor_GetPriority(app->actors[BOX2_ACTOR_ITEM_ICON]);
            u32 bgPriority = ClActor_GetBgPriority(app->actors[BOX2_ACTOR_ITEM_ICON]);

            for (i = 0; i < 8; i++) {
                Box2Obj_SetPos(app, BOX2_ACTOR_ITEM_OUTLINE + i, x + sItemOutlineX[i], y + sItemOutlineY[i],
                               CLACT_SURFACE_MAIN);
                SetSoftPriority(app, BOX2_ACTOR_ITEM_OUTLINE + i, priority + 1);
                SetBgPriority(app, BOX2_ACTOR_ITEM_OUTLINE + i, bgPriority);
                Box2Obj_SetVisible(app, BOX2_ACTOR_ITEM_OUTLINE + i, FALSE);
            }
        }
    }
}

void Box2Obj_ShowItemOutline(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < 8; i++) {
        Box2Obj_SetVisible(app, BOX2_ACTOR_ITEM_OUTLINE + i, TRUE);
    }
}

void Box2Obj_MoveItemOutline(Box2AppWork *app) {
    s16 x;
    s16 y;
    u32 i;

    if (app->actors[BOX2_ACTOR_ITEM_OUTLINE] != NULL) {
        Box2Obj_GetPos(app, BOX2_ACTOR_ITEM_ICON, &x, &y, CLACT_SURFACE_MAIN);
        for (i = 0; i < 8; i++) {
            Box2Obj_SetPos(app, BOX2_ACTOR_ITEM_OUTLINE + i, x + sItemOutlineX[i], y + sItemOutlineY[i],
                           CLACT_SURFACE_MAIN);
        }
    }
}

static void CreateFixedActors(Box2SysWork *syswk) {
    Box2AppWork *app = syswk->app;
    u32 i;

    app->actors[BOX2_ACTOR_SCROLL_LEFT] = CreateActor(app, &sActorData[BOX2_ACTOR_SCROLL_LEFT]);
    app->actors[BOX2_ACTOR_SCROLL_RIGHT] = CreateActor(app, &sActorData[BOX2_ACTOR_SCROLL_RIGHT]);
    app->actors[BOX2_ACTOR_TRAY_HIGHLIGHT] = CreateActor(app, &sActorData[BOX2_ACTOR_TRAY_HIGHLIGHT]);
    app->actors[BOX2_ACTOR_BOX_NAME_FRAME] = CreateActor(app, &sActorData[BOX2_ACTOR_BOX_NAME_FRAME]);
    app->actors[BOX2_ACTOR_CURSOR] = CreateActor(app, &sActorData[BOX2_ACTOR_CURSOR]);
    app->actors[BOX2_ACTOR_UNUSED] = CreateActor(app, &sActorData[BOX2_ACTOR_UNUSED]);
    app->actors[BOX2_ACTOR_ITEM_ICON] = CreateActor(app, &sActorData[BOX2_ACTOR_ITEM_ICON]);
    app->actors[BOX2_ACTOR_BACK_BUTTON] = CreateActor(app, &sActorData[BOX2_ACTOR_BACK_BUTTON]);
    app->actors[BOX2_ACTOR_CLOSE_BUTTON] = CreateActor(app, &sActorData[BOX2_ACTOR_CLOSE_BUTTON]);
    app->actors[BOX2_ACTOR_SUMMARY_BUTTON] = CreateActor(app, &sActorData[BOX2_ACTOR_SUMMARY_BUTTON]);
    app->actors[BOX2_ACTOR_MENU_MODE] = CreateActor(app, &sActorData[BOX2_ACTOR_MENU_MODE]);
    app->actors[BOX2_ACTOR_SINGLE_MODE] = CreateActor(app, &sActorData[BOX2_ACTOR_SINGLE_MODE]);
    app->actors[BOX2_ACTOR_RANGE_MODE] = CreateActor(app, &sActorData[BOX2_ACTOR_RANGE_MODE]);
    for (i = 0; i < 6; i++) {
        app->actors[BOX2_ACTOR_MARK_MAIN + i] = CreateActor(app, &sActorData[BOX2_ACTOR_MARK_MAIN + i]);
        app->actors[BOX2_ACTOR_MARK_SUB + i] = CreateActor(app, &sActorData[BOX2_ACTOR_MARK_SUB + i]);
    }
    app->actors[BOX2_ACTOR_SHINY] = CreateActor(app, &sActorData[BOX2_ACTOR_SHINY]);
    app->actors[BOX2_ACTOR_POKERUS_CURED] = CreateActor(app, &sActorData[BOX2_ACTOR_POKERUS_CURED]);
    app->actors[BOX2_ACTOR_POKERUS_ACTIVE] = CreateActor(app, &sActorData[BOX2_ACTOR_POKERUS_ACTIVE]);
    app->actors[BOX2_ACTOR_SEARCH_BUTTON] = CreateActor(app, &sActorData[BOX2_ACTOR_SEARCH_BUTTON]);
    app->actors[BOX2_ACTOR_RANGE_COUNT] = CreateActor(app, &sActorData[BOX2_ACTOR_RANGE_COUNT]);
    Box2Obj_SetVisible(app, BOX2_ACTOR_UNUSED, FALSE);
    Box2Obj_SetVisible(app, BOX2_ACTOR_ITEM_ICON, FALSE);
    Box2Obj_SetVisible(app, BOX2_ACTOR_SHINY, FALSE);
    Box2Obj_SetVisible(app, BOX2_ACTOR_POKERUS_CURED, FALSE);
    Box2Obj_SetVisible(app, BOX2_ACTOR_POKERUS_ACTIVE, FALSE);
    Box2Obj_SetVisible(app, BOX2_ACTOR_RANGE_COUNT, FALSE);
    for (i = 0; i < 16; i++) {
        app->actors[BOX2_ACTOR_RANGE_DOT + i] = CreateActor(app, &sRangeDotData);
        Box2Obj_SetVisible(app, BOX2_ACTOR_RANGE_DOT + i, FALSE);
    }
    Box2Obj_SetToolButtons(syswk, BOX2_ACTOR_MENU_MODE, TRUE);
}

void Box2Obj_SetToolButtons(Box2SysWork *syswk, unsigned int set, BOOL visible) {
    int i;

    if (syswk->param->mode != BOX2_MODE_MOVE_POKEMON && syswk->param->mode != BOX2_MODE_BATTLE_BOX &&
        syswk->param->mode != BOX2_MODE_MOVE_ITEMS) {
        for (i = 9; i <= 11; i++) {
            Box2Obj_SetVisible(syswk->app, i, FALSE);
        }
    } else {
        for (i = 0; i < 3; i++) {
            Box2Obj_SetAnim(syswk->app, 9 + i, sButtonAnims[set - 9][i]);
            Box2Obj_SetVisible(syswk->app, 9 + i, visible);
        }
        if (syswk->param->mode == BOX2_MODE_MOVE_ITEMS) {
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_RANGE_MODE, FALSE);
        }
    }
}

void Box2Obj_SetCursorAnim(Box2SysWork *syswk, u32 anim) {
    if (syswk->moveMode == BOX2_MOVE_MODE_SINGLE) {
        anim += 4;
    } else if (syswk->moveMode == BOX2_MOVE_MODE_RANGE) {
        anim += 9;
    }
    Box2Obj_SetAnim(syswk->app, BOX2_ACTOR_CURSOR, anim);
}

void Box2Obj_ShowCursor(Box2SysWork *syswk, BOOL show) {
    if (show == TRUE) {
        if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, TRUE);
        }
    } else {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
    }
}

void Box2Obj_PutPokeOutline(Box2SysWork *syswk) {
    Box2Obj_PutPokeOutlineAt(syswk, syswk->pos);
}

void Box2Obj_PutPokeOutlineAt(Box2SysWork *syswk, u32 pos) {
    Box2ActorData data;
    NNSG2dImageProxy proxy;
    s16 x;
    s16 y;
    Box2AppWork *app;
    u32 id;
    u16 i;

    if (pos != BOX2_GET_NONE) {
        app = syswk->app;
        id = app->pokeIconId[pos];
        Box2Obj_GetPos(app, id, &x, &y, CLACT_SURFACE_MAIN);
        if (app->actors[BOX2_ACTOR_ITEM_OUTLINE] == NULL) {
            data = sPokeIconData;
            data.setup.priority = ClActor_GetPriority(app->actors[id]) + 1;
            data.setup.bgPriority = ClActor_GetBgPriority(app->actors[id]);
            data.chr = id - BOX2_ACTOR_POKEICON;
            data.pal = BOX2_PAL_BOX_UI;
            data.palette = 1;
            for (i = 0; i < 8; i++) {
                data.setup.x = x + sItemOutlineX[i];
                data.setup.y = y + sItemOutlineY[i];
                app->actors[BOX2_ACTOR_ITEM_OUTLINE + i] = CreateActor(app, &data);
                ClActor_SetPalette(app->actors[BOX2_ACTOR_ITEM_OUTLINE + i], (u8)data.palette, 1);
            }
        } else {
            u32 priority;
            u32 bgPriority;

            ClActor_GetImageProxy(app->actors[id], &proxy);
            priority = ClActor_GetPriority(app->actors[id]);
            bgPriority = ClActor_GetBgPriority(app->actors[id]);
            for (i = 0; i < 8; i++) {
                ClActor_SetImageProxy(app->actors[BOX2_ACTOR_ITEM_OUTLINE + i], &proxy);
                Box2Obj_SetPos(app, BOX2_ACTOR_ITEM_OUTLINE + i, x + sItemOutlineX[i], y + sItemOutlineY[i],
                               CLACT_SURFACE_MAIN);
                SetSoftPriority(app, BOX2_ACTOR_ITEM_OUTLINE + i, priority + 1);
                SetBgPriority(app, BOX2_ACTOR_ITEM_OUTLINE + i, bgPriority);
                Box2Obj_SetVisible(app, BOX2_ACTOR_ITEM_OUTLINE + i, TRUE);
            }
        }
    }
}

void Box2Obj_ShowPokeOutline(Box2SysWork *syswk, BOOL visible) {
    u32 i;
    u32 id;

    for (i = 0; i < 8; i++) {
        if (syswk->app->actors[BOX2_ACTOR_ITEM_OUTLINE + i] != NULL) {
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i, visible);
            if (visible == TRUE && syswk->pos != BOX2_GET_NONE) {
                id = syswk->app->pokeIconId[syswk->pos];
                SetSoftPriority(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i,
                                ClActor_GetPriority(syswk->app->actors[id]) + 1);
                SetBgPriority(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i, ClActor_GetBgPriority(syswk->app->actors[id]));
            }
        }
    }
}

void Box2Obj_UpdateOutlinePriority(Box2SysWork *syswk, u32 pos) {
    u32 id = syswk->app->pokeIconId[pos];
    u32 priority = ClActor_GetPriority(syswk->app->actors[id]);
    u32 bgPriority = ClActor_GetBgPriority(syswk->app->actors[id]);
    u32 i;

    for (i = 0; i < 8; i++) {
        if (syswk->app->actors[BOX2_ACTOR_ITEM_OUTLINE + i] != NULL && syswk->pos != BOX2_GET_NONE) {
            SetSoftPriority(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i, priority + 1);
            SetBgPriority(syswk->app, BOX2_ACTOR_ITEM_OUTLINE + i, bgPriority);
        }
    }
}

void Box2Obj_MoveOutlineToIcon(Box2AppWork *app, u32 pos) {
    s16 x;
    s16 y;
    u32 i;

    if (pos != BOX2_GET_NONE) {
        Box2Obj_GetPos(app, app->pokeIconId[pos], &x, &y, CLACT_SURFACE_MAIN);
        for (i = 0; i < 8; i++) {
            Box2Obj_SetPos(app, BOX2_ACTOR_ITEM_OUTLINE + i, x + sItemOutlineX[i], y + sItemOutlineY[i],
                           CLACT_SURFACE_MAIN);
        }
    }
}

static void CreateTrayIconActors(Box2AppWork *app) {
    Box2ActorData data;
    Box2ActorData base = sTrayIconData;
    u32 i;

    for (i = 0; i < BOX2_BOX_LIST_DRAW_ROWS; i++) {
        data = base;
        data.setup.y += (s16)(i * 34);
        data.chr = BOX2_CHAR_TRAY_ICON_FIRST + i;
        app->actors[BOX2_ACTOR_TRAY_ICON + i] = CreateActor(app, &data);
    }
}

void Box2Obj_ShowScrollArrows(Box2AppWork *app, BOOL visible) {
    Box2Obj_SetVisible(app, BOX2_ACTOR_SCROLL_LEFT, visible);
    Box2Obj_SetVisible(app, BOX2_ACTOR_SCROLL_RIGHT, visible);
}

void Box2Obj_MoveMarkIcons(Box2SysWork *syswk) {
    s8 x;
    s8 y;
    s16 px;
    s16 py;
    u32 i;

    BGWinFrame_GetPos(syswk->app->bgWinFrame, BOX2_FRAME_MARKINGS, &x, &y);
    py = y * 8 + 8;
    px = x * 8 + 23;
    for (i = 0; i < BOX2_MARK_COUNT; i++) {
        Box2Obj_SetPos(syswk->app, 15 + i, px + (i % 2) * 32, py + (i / 2) * 24, CLACT_SURFACE_MAIN);
    }
}

void Box2Obj_ResetBoxList(Box2SysWork *syswk) {
    Box2Obj_MakeTrayIconChar(syswk, syswk->tray);
    PlaceTrayIcons(syswk);
}

void Box2Obj_ScrollBoxListX(Box2SysWork *syswk, s32 mv) {
    s16 x;
    s16 y;
    u32 i;

    Box2Obj_GetPos(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT, &x, &y, CLACT_SURFACE_MAIN);
    Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT, x - mv, y, CLACT_SURFACE_MAIN);
    for (i = 0; i < BOX2_BOX_LIST_DRAW_ROWS; i++) {
        Box2Obj_GetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, &x, &y, CLACT_SURFACE_MAIN);
        Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, x - mv, y, CLACT_SURFACE_MAIN);
        BmpOam_ActorGetPos(syswk->app->fontOam[BOX2_FONT_OAM_BOX_COUNT_FIRST + i].oam, &x, &y);
        BmpOam_ActorSetPos(syswk->app->fontOam[BOX2_FONT_OAM_BOX_COUNT_FIRST + i].oam, x - mv, y);
    }
}

void Box2Obj_HideTrayHighlight(Box2AppWork *app) {
    Box2Obj_SetPos(app, BOX2_ACTOR_TRAY_HIGHLIGHT, 300, 34, CLACT_SURFACE_MAIN);
}

static void PlaceTrayIcons(Box2SysWork *syswk) {
    Box2ActorData data;
    Box2ActorData base = sTrayIconData;
    u32 tray;
    s16 i;

    for (i = 0; i < BOX2_BOX_LIST_DRAW_ROWS; i++) {
        tray = Box2Main_GetTrayScroll(syswk, i - 1);
        data = base;
        data.setup.y += (s16)(i * 34);
        Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, data.setup.x, data.setup.y, CLACT_SURFACE_MAIN);
        SetTrayIconChar(syswk, tray, BOX2_ACTOR_TRAY_ICON + i);
        BmpOam_ActorSetPos(syswk->app->fontOam[BOX2_FONT_OAM_BOX_COUNT_FIRST + i].oam, 316, i * 34 + 2);
        Box2Bmp_PutTrayCount(syswk, tray, i + 1);
    }
}

void Box2Obj_GetTrayIconPos(Box2AppWork *app, u32 row, s16 *x, s16 *y) {
    u32 i;

    row = (row + 1) * 34;
    for (i = 0; i < BOX2_BOX_LIST_DRAW_ROWS; i++) {
        Box2Obj_GetPos(app, BOX2_ACTOR_TRAY_ICON + i, x, y, CLACT_SURFACE_MAIN);
        if (row == *y) {
            return;
        }
    }
    *x = 0;
    *y = 0;
}

void Box2Obj_MakeTrayIconChar(Box2SysWork *syswk, u32 tray) {
    NNSG2dCharacterData *chr;
    void *buf = GFL_G2DIOReadOBJNCGR(ARCID_BOX2, 71, TRUE, &chr, HEAPID_BOX2_APP);

    MI_CpuCopy8(chr->pRawData, syswk->app->trayIconChar[tray], 0x400);
    GFL_HeapFree(buf);
    ReplaceTrayIconColor(syswk, syswk->app->trayIconChar[tray], Box2Main_GetWallPaperNumber(syswk, tray), 8, 0x400);
    DrawTrayIconPokes(syswk, tray, syswk->app->trayIconChar[tray]);
}

void Box2Obj_MakeAllTrayIconChars(Box2SysWork *syswk) {
    u32 i;

    for (i = 0; i < BOX2_TRAY_MAX; i++) {
        Box2Obj_MakeTrayIconChar(syswk, i);
    }
}

void Box2Obj_RefreshTrayIcon(Box2SysWork *syswk, u32 tray) {
    s16 i;

    for (i = 0; i < BOX2_BOX_LIST_DRAW_ROWS; i++) {
        u32 scroll = Box2Main_GetTrayScroll(syswk, i - 1);

        if (scroll == tray) {
            RefreshTrayIconRow(syswk, tray, i);
            return;
        }
    }
}

static void SetTrayIconChar(Box2SysWork *syswk, u32 tray, u32 id) {
    Box2AppWork *app = syswk->app;

    ClActSys_WriteCharData(ClActor_GetCharIdx(app->actors[id]), app->trayIconChar[tray], 0x400, 0, CLACT_VRAM_MAIN);
}

static void RefreshTrayIconRow(Box2SysWork *syswk, u32 tray, u32 row) {
    s16 x;
    s16 y;
    u32 i;

    row *= 34;
    for (i = 0; i < BOX2_BOX_LIST_DRAW_ROWS; i++) {
        Box2Obj_GetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, &x, &y, CLACT_SURFACE_MAIN);
        if (row == y) {
            SetTrayIconChar(syswk, tray, BOX2_ACTOR_TRAY_ICON + i);
            Box2Bmp_PutTrayCount(syswk, tray, i + 1);
            return;
        }
    }
}

static void ReplaceTrayIconColor(Box2SysWork *syswk, u8 *chr, u32 wallpaper, u32 color, u32 size) {
    u32 i;

    for (i = 0; i < size; i++) {
        if (chr[i] == color) {
            chr[i] = sTrayIconColors[wallpaper];
        }
    }
}

static void DrawTrayIconPokes(Box2SysWork *syswk, u32 tray, u8 *chr) {
    BoxPkm *pkm;
    BOOL encrypted;
    u8 j;
    u8 i;
    u8 px;
    u8 py;
    u32 species;
    PersonalData *personal;
    u8 y;
    u16 color;
    u16 form;

    py = 11;
    for (j = 0; j < 5; j++) {
        px = 10;
        for (i = 0; i < 6; i++) {
            pkm = BoxSaveAccessor_GetPkm(syswk->param->boxes, tray, i + j * 6);
            encrypted = PML_PkmDecrypt(pkm);
            species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
            if (PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                if (PML_PkmGetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 0) {
                    form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
                    personal = PML_PersonalLoad(species, form, HEAPID_BOX2_APP);
                    color = PML_PersonalGetParam(personal, PERSONAL_COLOR);
                    PML_PersonalFree(personal);
                } else if (species == SPECIES_MANAPHY) {
                    color = 1;
                } else {
                    color = 8;
                }
                color += 16;
                color = (color << 8) | color;
                for (y = py; y < py + 2; y++) {
                    MIi_CpuClear16(color, &chr[((y & 7) << 3) + ((((y >> 3) << 2) + (px >> 3)) << 6) + (px & 7)], 2);
                }
            }
            PML_PkmReEncrypt(pkm, encrypted);
            px += 2;
        }
        py += 2;
    }
}

void Box2Obj_ScrollBoxListY(Box2SysWork *syswk, s16 mv) {
    s16 x;
    s16 y;
    s16 fx;
    s16 fy;
    s32 speed;
    s16 ny;
    s16 nfy;
    u32 i;

    speed = (mv < 0 ? -mv : mv) == 5 ? 2 : 8;
    if (mv < 0) {
        speed *= -1;
    }
    for (i = 0; i < BOX2_BOX_LIST_DRAW_ROWS; i++) {
        Box2Obj_GetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, &x, &y, CLACT_SURFACE_MAIN);
        ny = y + speed;
        Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, x, ny, CLACT_SURFACE_MAIN);
        BmpOam_ActorGetPos(syswk->app->fontOam[BOX2_FONT_OAM_BOX_COUNT_FIRST + i].oam, &fx, &fy);
        nfy = fy + speed;
        BmpOam_ActorSetPos(syswk->app->fontOam[BOX2_FONT_OAM_BOX_COUNT_FIRST + i].oam, fx, nfy);
        if (ny < -16) {
            u32 tray = Box2Main_GetTrayScroll(syswk, 4);

            SetTrayIconChar(syswk, tray, BOX2_ACTOR_TRAY_ICON + i);
            Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, x, ny + 204, CLACT_SURFACE_MAIN);
            Box2Bmp_PutTrayCount(syswk, tray, i + 1);
            BmpOam_ActorSetPos(syswk->app->fontOam[BOX2_FONT_OAM_BOX_COUNT_FIRST + i].oam, fx, nfy + 204);
        } else if (ny >= 186) {
            u32 tray = Box2Main_GetTrayScroll(syswk, -1);

            SetTrayIconChar(syswk, tray, BOX2_ACTOR_TRAY_ICON + i);
            Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_ICON + i, x, ny - 204, CLACT_SURFACE_MAIN);
            Box2Bmp_PutTrayCount(syswk, tray, i + 1);
            BmpOam_ActorSetPos(syswk->app->fontOam[BOX2_FONT_OAM_BOX_COUNT_FIRST + i].oam, fx, nfy - 204);
        }
    }
    if (Box2Obj_IsVisible(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT) == TRUE) {
        Box2Obj_GetPos(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT, &x, &y, CLACT_SURFACE_MAIN);
        Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT, x, y + speed, CLACT_SURFACE_MAIN);
    }
}

void Box2Obj_StartHighlightScroll(Box2SysWork *syswk, s32 mv) {
    s16 x;
    s16 y;

    if (mv < 0 && Box2Main_GetTrayScroll(syswk, -1) == syswk->tray) {
        Box2Obj_GetPos(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT, &x, &y, CLACT_SURFACE_MAIN);
        Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT, x, -34, CLACT_SURFACE_MAIN);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT, TRUE);
    } else if (mv > 0 && Box2Main_GetTrayScroll(syswk, 4) == syswk->tray) {
        Box2Obj_GetPos(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT, &x, &y, CLACT_SURFACE_MAIN);
        Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT, x, 204, CLACT_SURFACE_MAIN);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT, TRUE);
    }
}

void Box2Obj_PlaceTrayHighlight(Box2SysWork *syswk) {
    s16 x;
    s16 y;
    u32 i;

    for (i = 0; i < BOX2_BOX_LIST_DRAW_ROWS; i++) {
        if (Box2Main_GetTrayScroll(syswk, i - 1) == syswk->tray) {
            Box2Obj_GetPos(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT, &x, &y, CLACT_SURFACE_MAIN);
            Box2Obj_SetPos(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT, x, i * 34, CLACT_SURFACE_MAIN);
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT, TRUE);
            return;
        }
    }
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_TRAY_HIGHLIGHT, FALSE);
}

static void PutBoxNameFrame(Box2SysWork *syswk, u32 index) {
    s16 y = sBoxNameY[index];

    Box2Obj_SetPos(syswk->app, BOX2_ACTOR_BOX_NAME_FRAME, 206, y, CLACT_SURFACE_MAIN);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_BOX_NAME_FRAME, TRUE);
    BmpOam_ActorSetPos(syswk->app->fontOam[BOX2_FONT_OAM_BOX_NAME].oam, 158, y - 8);
    BmpOam_ActorSetDrawEnable(syswk->app->fontOam[BOX2_FONT_OAM_BOX_NAME].oam, TRUE);
}

void Box2Obj_ShowBoxNameFrame(Box2SysWork *syswk, u32 index, BOOL show) {
    if (show == TRUE) {
        Box2Bmp_DrawScrolledTrayName(syswk, index);
        PutBoxNameFrame(syswk, index);
    } else {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_BOX_NAME_FRAME, FALSE);
        BmpOam_ActorSetDrawEnable(syswk->app->fontOam[BOX2_FONT_OAM_BOX_NAME].oam, FALSE);
    }
}

void Box2Obj_SetButtons(Box2SysWork *syswk, u8 backButtonState, u8 closeButtonState, u8 summaryButtonState,
                        u8 searchButtonState) {
    syswk->closeButtonState = closeButtonState;
    syswk->summaryButtonState = summaryButtonState;
    syswk->backButtonState = backButtonState;
    syswk->searchButtonState = searchButtonState;
    if (backButtonState == BOX2_BUTTON_ENABLED) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_BACK_BUTTON, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, BOX2_ACTOR_BACK_BUTTON, 1);
    } else if (backButtonState == BOX2_BUTTON_HIDDEN) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_BACK_BUTTON, FALSE);
    } else {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_BACK_BUTTON, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, BOX2_ACTOR_BACK_BUTTON, 15);
    }
    if (closeButtonState == BOX2_BUTTON_ENABLED) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CLOSE_BUTTON, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, BOX2_ACTOR_CLOSE_BUTTON, 0);
    } else if (closeButtonState == BOX2_BUTTON_HIDDEN) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CLOSE_BUTTON, FALSE);
    } else {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CLOSE_BUTTON, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, BOX2_ACTOR_CLOSE_BUTTON, 14);
    }
    if (summaryButtonState == BOX2_BUTTON_ENABLED) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_SUMMARY_BUTTON, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, BOX2_ACTOR_SUMMARY_BUTTON, 0);
    } else if (summaryButtonState == BOX2_BUTTON_HIDDEN) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_SUMMARY_BUTTON, FALSE);
    } else {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_SUMMARY_BUTTON, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, BOX2_ACTOR_SUMMARY_BUTTON, 2);
    }
    if (searchButtonState == BOX2_BUTTON_ENABLED) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_SEARCH_BUTTON, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, BOX2_ACTOR_SEARCH_BUTTON, 0);
    } else if (searchButtonState == BOX2_BUTTON_HIDDEN) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_SEARCH_BUTTON, FALSE);
    } else {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_SEARCH_BUTTON, TRUE);
        Box2Obj_SetAnimAndPlay(syswk->app, BOX2_ACTOR_SEARCH_BUTTON, 2);
    }
}

void Box2Obj_HideButtons(Box2SysWork *syswk) {
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_BACK_BUTTON, FALSE);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CLOSE_BUTTON, FALSE);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_SUMMARY_BUTTON, FALSE);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_SEARCH_BUTTON, FALSE);
}

static void CreateFontOams(Box2AppWork *app) {
    BmpOamActorSetup setup;
    u32 i;
    Box2FontOam *font;

    app->bmpOam = BmpOam_Init(HEAPID_BOX2_APP, app->clunit);
    app->fontOam[BOX2_FONT_OAM_BOX_NAME].bitmap = GFL_BitmapCreate(12, 2, 0x20, HEAPID_BOX2_APP);
    setup.bitmap = app->fontOam[BOX2_FONT_OAM_BOX_NAME].bitmap;
    setup.x = 0;
    setup.y = 0;
    setup.palette = app->palRes[BOX2_PAL_BOX_UI];
    setup.paletteOffset = 0;
    setup.priority = 30;
    setup.bgPriority = 0;
    setup.surface = 0xffff;
    setup.vramType = CLACT_VRAM_MAIN;
    app->fontOam[BOX2_FONT_OAM_BOX_NAME].oam = BmpOam_ActorAdd(app->bmpOam, &setup);
    BmpOam_ActorSetDrawEnable(app->fontOam[BOX2_FONT_OAM_BOX_NAME].oam, FALSE);
    for (i = BOX2_FONT_OAM_TRAY_NAME_FIRST; i <= BOX2_FONT_OAM_TRAY_NAME_SECOND; i++) {
        font = &app->fontOam[i];
        font->bitmap = GFL_BitmapCreate(12, 2, 0x20, HEAPID_BOX2_APP);
        setup.bitmap = font->bitmap;
        setup.x = 36;
        setup.y = 20;
        setup.bgPriority = 3;
        font->oam = BmpOam_ActorAdd(app->bmpOam, &setup);
    }
    for (i = 0; i < BOX2_BOX_LIST_DRAW_ROWS; i++) {
        font = &app->fontOam[BOX2_FONT_OAM_BOX_COUNT_FIRST + i];
        font->bitmap = GFL_BitmapCreate(2, 1, 0x20, HEAPID_BOX2_APP);
        setup.bitmap = font->bitmap;
        setup.x = 316;
        setup.y = i * 34 + 2;
        setup.bgPriority = 1;
        font->oam = BmpOam_ActorAdd(app->bmpOam, &setup);
    }
    font = &app->fontOam[BOX2_FONT_OAM_RANGE_COUNT];
    font->bitmap = GFL_BitmapCreate(2, 2, 0x20, HEAPID_BOX2_APP);
    setup.bitmap = font->bitmap;
    setup.x = 0;
    setup.y = 0;
    setup.priority = 45;
    setup.bgPriority = 0;
    font->oam = BmpOam_ActorAdd(app->bmpOam, &setup);
    Box2Obj_SetFontOamVisible(app, BOX2_FONT_OAM_RANGE_COUNT, FALSE);
}

static void DeleteFontOams(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < NELEMS(app->fontOam); i++) {
        BmpOam_ActorDel(app->fontOam[i].oam);
        GFL_BitmapFree(app->fontOam[i].bitmap);
    }
    BmpOam_Exit(app->bmpOam);
}

void Box2Obj_SetFontOamVisible(Box2AppWork *app, u32 index, BOOL visible) {
    BmpOam_ActorSetDrawEnable(app->fontOam[index].oam, visible);
}

BOOL Box2Obj_IsFontOamVisible(Box2AppWork *app, u32 index) {
    return BmpOam_ActorGetDrawEnable(app->fontOam[index].oam);
}

void Box2Obj_PutFontOamOffscreen(Box2AppWork *app, u32 index, u32 dir) {
    if (dir == 0) {
        BmpOam_ActorSetPos(app->fontOam[index].oam, -148, 20);
    } else {
        BmpOam_ActorSetPos(app->fontOam[index].oam, 220, 20);
    }
}

void Box2Obj_ScrollFontOams(Box2AppWork *app, s32 mv) {
    s16 x;
    s16 y;
    u32 i;

    for (i = BOX2_FONT_OAM_TRAY_NAME_FIRST; i <= BOX2_FONT_OAM_TRAY_NAME_SECOND; i++) {
        BmpOam_ActorGetPos(app->fontOam[i].oam, &x, &y);
        BmpOam_ActorSetPos(app->fontOam[i].oam, x + mv, y);
        if (x + mv == -148 || x + mv == 220) {
            Box2Obj_SetFontOamVisible(app, i, FALSE);
        }
    }
}

static void InitRangeSelect(Box2SysWork *syswk) {
    MI_CpuFill8(&syswk->app->rangeSelect, 0, sizeof(Box2RangeSelect));
    syswk->app->rangeSelect.anmMax = 384;
}

void Box2Obj_SetRangeAnm(Box2SysWork *syswk, u32 anm) {
    syswk->app->rangeSelect.anm = anm;
}

void Box2Obj_UpdateRangeDots(Box2SysWork *syswk) {
    s16 x;
    s16 y;
    s32 length;
    s32 half;
    s32 top;
    s32 right;
    s32 bottom;
    s32 left;
    s32 width;
    s32 height;
    s32 pos;
    s32 px;
    s32 py;
    int i;
    Box2RangeSelect *rs = &syswk->app->rangeSelect;

    if (rs->anm != rs->prevAnm) {
        for (i = 0; i < 16; i++) {
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_RANGE_DOT + i, rs->anm);
        }
    }
    rs->prevAnm = rs->anm;
    rs->anmCnt++;
    if (rs->anmCnt >= rs->anmMax) {
        rs->anmCnt = 0;
    }
    width = rs->right - rs->left;
    height = rs->bottom - rs->top;
    left = rs->left;
    top = rs->top;
    right = rs->right;
    bottom = rs->bottom;
    half = width + height;
    length = half * 2;
    if (syswk->pickState == BOX2_PICK_RANGE_HELD) {
        Box2Obj_GetPos(syswk->app, syswk->app->pokeIconId[BOX2_BOXLIST_POS], &x, &y, CLACT_SURFACE_MAIN);
        left = x - 14;
        top = y - 6;
        right = left + width;
        bottom = top + height;
    }
    for (i = 0; i < 16; i++) {
        pos = (rs->anmCnt + i * 24) * length / rs->anmMax % length;
        if (pos < half) {
            if (pos < width) {
                px = left + pos;
                py = top;
            } else {
                px = right;
                py = top + (pos - width);
            }
        } else {
            pos -= half;
            if (pos < width) {
                px = right - pos;
                py = bottom;
            } else {
                px = left;
                py = bottom - (pos - width);
            }
        }
        Box2Obj_SetPos(syswk->app, BOX2_ACTOR_RANGE_DOT + i, px, py, CLACT_SURFACE_MAIN);
    }
}

static void SortPair(s32 *a, s32 *b) {
    s32 tmp;

    if (*a > *b) {
        tmp = *a;
        *a = *b;
        *b = tmp;
    }
}

static void SetRangeRect(Box2SysWork *syswk, s32 left, s32 top, s32 right, s32 bottom) {
    Box2RangeSelect *rs = &syswk->app->rangeSelect;

    SortPair(&left, &right);
    SortPair(&top, &bottom);
    rs->left = left;
    rs->top = top;
    rs->right = right;
    rs->bottom = bottom;
}

static s32 Min4(s32 a, s32 b, s32 c, s32 d) {
    if (a <= b && a <= c && a <= d) {
        return a;
    }
    if (b <= a && b <= c && b <= d) {
        return b;
    }
    if (c <= a && c <= b && c <= d) {
        return c;
    }
    return d;
}

static s32 Max4(s32 a, s32 b, s32 c, s32 d) {
    if (a >= b && a >= c && a >= d) {
        return a;
    }
    if (b >= a && b >= c && b >= d) {
        return b;
    }
    if (c >= a && c >= b && c >= d) {
        return c;
    }
    return d;
}

static void GetTrayPokeRect(int pos, Box2Rect *rect) {
    rect->left = pos % 6 * 24 + 10;
    rect->top = pos / 6 * 24 + 42;
    rect->right = rect->left + 26;
    rect->bottom = rect->top + 22;
}

static void GetPartyPokeRect(int pos, Box2Rect *rect) {
    int col;
    int row;

    pos -= BOX2_PARTY_POS;
    col = pos % 2;
    row = pos / 2;
    rect->left = col * 40 + 178;
    if (pos & 1) {
        rect->top = row * 32 + 66;
    } else {
        rect->top = row * 32 + 58;
    }
    rect->right = rect->left + 26;
    rect->bottom = rect->top + 22;
}

static void GetPartyPokeRectCompact(int pos, Box2Rect *rect) {
    s32 x;
    s32 y;

    pos -= BOX2_PARTY_POS;
    x = pos % 2 * 24 + 178;
    y = pos / 2 * 24 + 58;
    rect->left = x;
    rect->top = y;
    rect->right = x + 26;
    rect->bottom = y + 22;
}

void Box2Obj_SetRangeRect(Box2SysWork *syswk, int start, int end, u32 mode) {
    Box2Rect rect1;
    Box2Rect rect2;
    s32 left;
    s32 top;
    s32 right;

    if (start < BOX2_TRAY_POKE_MAX) {
        GetTrayPokeRect(start, &rect1);
        GetTrayPokeRect(end, &rect2);
    } else if (mode == 1) {
        GetPartyPokeRectCompact(start, &rect1);
        GetPartyPokeRectCompact(end, &rect2);
    } else {
        GetPartyPokeRect(start, &rect1);
        GetPartyPokeRect(end, &rect2);
    }
    left = Min4(rect1.left, rect1.right, rect2.left, rect2.right);
    top = Min4(rect1.top, rect1.bottom, rect2.top, rect2.bottom);
    right = Max4(rect1.left, rect1.right, rect2.left, rect2.right);
    SetRangeRect(syswk, left, top, right, Max4(rect1.top, rect1.bottom, rect2.top, rect2.bottom));
}

static void GetTrayRangeCorners(int start, int end, s32 *x1, s32 *y1, s32 *x2, s32 *y2) {
    *x1 = start % 6;
    *y1 = start / 6;
    *x2 = end % 6;
    *y2 = end / 6;
    SortPair(x1, x2);
    SortPair(y1, y2);
}

static void GetPartyRangeCorners(int start, int end, s32 *x1, s32 *y1, s32 *x2, s32 *y2) {
    *x1 = start % 2;
    *y1 = start / 2;
    *x2 = end % 2;
    *y2 = end / 2;
    SortPair(x1, x2);
    SortPair(y1, y2);
}

void Box2Obj_GetRangeSize(int start, int end, u32 *width, u32 *height) {
    s32 x1;
    s32 y1;
    s32 x2;
    s32 y2;

    if (start >= BOX2_TRAY_POKE_MAX) {
        GetPartyRangeCorners(start - BOX2_TRAY_POKE_MAX, end - BOX2_TRAY_POKE_MAX, &x1, &y1, &x2, &y2);
    } else {
        GetTrayRangeCorners(start, end, &x1, &y1, &x2, &y2);
    }
    *width = x2 - x1 + 1;
    *height = y2 - y1 + 1;
}

u32 Box2Obj_GetTrayRangeStart(int start, int end) {
    s32 x1;
    s32 y1;
    s32 x2;
    s32 y2;

    GetTrayRangeCorners(start, end, &x1, &y1, &x2, &y2);
    return x1 + y1 * 6;
}

u32 Box2Obj_GetPartyRangeStart(int start, int end) {
    s32 x1;
    s32 y1;
    s32 x2;
    s32 y2;

    GetPartyRangeCorners(start - BOX2_TRAY_POKE_MAX, end - BOX2_TRAY_POKE_MAX, &x1, &y1, &x2, &y2);
    return x1 + y1 * 2 + BOX2_TRAY_POKE_MAX;
}

void Box2Obj_ResetRangeIconPriority(Box2AppWork *app, u32 pos, u32 width, u32 height, BOOL put) {
    u32 id;
    u32 i;
    u32 j;

    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            id = app->pokeIconId[BOX2_BOXLIST_POS + j * width + i];
            if (put == FALSE) {
                SetBgPriority(app, id, 0);
                SetSoftPriority(app, id, 20);
            }
        }
    }
}

static BOOL IsInPickedRange(Box2SysWork *syswk, u32 pos) {
    u32 j;

    if (syswk->pos >= BOX2_TRAY_POKE_MAX) {
        return FALSE;
    }
    for (j = 0; j < syswk->app->rangeHeight; j++) {
        if (pos >= syswk->pos + j * 6 && pos < syswk->app->rangeWidth + (syswk->pos + j * 6)) {
            return TRUE;
        }
    }
    return FALSE;
}

void Box2Obj_ShowRangeCount(Box2AppWork *app, BOOL visible) {
    Box2Obj_SetVisible(app, BOX2_ACTOR_RANGE_COUNT, visible);
    Box2Obj_SetFontOamVisible(app, BOX2_FONT_OAM_RANGE_COUNT, visible);
}

void Box2Obj_PutRangeCount(Box2AppWork *app, s16 x, s16 y) {
    Box2Obj_SetPos(app, BOX2_ACTOR_RANGE_COUNT, x, y, CLACT_SURFACE_MAIN);
    BmpOam_ActorSetPos(app->fontOam[BOX2_FONT_OAM_RANGE_COUNT].oam, x - 16, y - 16);
}

void Box2Obj_ShowRangeIcons(Box2SysWork *syswk, BOOL visible) {
    u32 rowWidth;
    u16 j;
    u16 i;

    if (syswk->moveMode == BOX2_MOVE_MODE_RANGE) {
        Box2Obj_ShowRangeCount(syswk->app, visible);
        visible ^= 1;
        rowWidth = Box2Main_GetRowWidth(syswk, syswk->app->rangeSelect.startPos);
        for (j = 0; j < syswk->app->rangeHeight; j++) {
            for (i = 0; i < syswk->app->rangeWidth; i++) {
                if (syswk->app->rangeFlags[j * rowWidth + i] != 0) {
                    Box2Obj_SetVisible(
                        syswk->app, syswk->app->pokeIconId[BOX2_BOXLIST_POS + j * syswk->app->rangeWidth + i], visible);
                }
            }
        }
        Box2Obj_SetRangeAnm(syswk, visible);
    }
}
