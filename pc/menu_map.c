#include "app/townmap.h"
#include "dex_media.h"
#include "gfl/key.h"
#include "gx_backend.h"
#include "menu_apps.h"
#include "ui_frame.h"
#include "ui_widgets.h"

static Font *font;
static PcUiInput input;
static u32 mapPlaces[85], mapCount, mapCursor, mapStart;
static void RefreshMapPlaces(const TownMapNativeState *state) {
    mapCount = 0;
    for (u32 i = 0; i < state->count; i++) {
        TownMapNativePlace place;
        if (TownMap_GetNativePlace(i, &place) && place.visible && (state->mode == TOWNMAP_MODE_VIEW || place.canFly)) {
            if (i == state->selected)
                mapCursor = mapCount;
            if (mapCount < SDL_arraysize(mapPlaces))
                mapPlaces[mapCount++] = i;
        }
    }
    if (mapCursor >= mapCount)
        mapCursor = 0;
}

static void DrawMap(TownMapNativeState *state) {
    font = state->font;
    RefreshMapPlaces(state);
    PcUiFrame_Draw(font, PC_MENU_MAIL, "UNOVA", state->mode == TOWNMAP_MODE_VIEW ? "TOWN MAP" : "FLY",
                   "Click a destination   Up/Down: Browse   Z: Fly   X: Back");
    PcUi_Tab(font, "DESTINATIONS", (PcUiRect){ 28, 101, 203, 43 }, true, (PcUiAction){ 0 });
    PcUi_LineLabel(font, "UNOVA REGION", 65, 174, 800, PC_UI_INK, PC_UI_TEXT_CARD_TITLE);
    const u16 *pixels = PcDexMedia_Map();
    if (pixels) {
        SDL_memcpy(PcGx.textures, pixels, 256 * 256 * sizeof(u16));
        u32 texture = (7u << 26) | (5u << 23) | (5u << 20);
        PcUi_Quad(128, 215, 614.4f, 403.2f, -0.4f, 1, 1, 1, texture, 0, 0, 256, 168);
    }
    for (u32 i = 0; i < state->count; i++) {
        TownMapNativePlace place;
        if (!TownMap_GetNativePlace(i, &place) || !place.visible)
            continue;
        float x = 128 + place.x * 2.4f, y = 215 + place.y * 2.4f;
        if (y > 623)
            continue;
        u32 color = place.canFly ? 0x00cfe3 : 0x81919b;
        PcUi_Solid(x - 3, y - 3, 6, 6, color, 1);
        for (u32 row = 0; state->ready && row < mapCount; row++)
            if (mapPlaces[row] == i)
                PcUi_Control((PcUiShape){ .rect = { x - 12, y - 12, 24, 24 } }, (PcUiAction){ PC_UI_ACTION_ROW, row });
        if (i == state->selected) {
            PcUi_Solid(x - 8, y - 8, 16, 2, 0xffffff, 1);
            PcUi_Solid(x - 8, y + 6, 16, 2, 0xffffff, 1);
            PcUi_Solid(x - 8, y - 6, 2, 12, 0xffffff, 1);
            PcUi_Solid(x + 6, y - 6, 2, 12, 0xffffff, 1);
        }
    }
    PcUi_LineLabel(font, state->mode == TOWNMAP_MODE_VIEW ? "LOCATIONS" : "AVAILABLE FLIGHTS", 905, 180, 1224,
                   PC_UI_CYAN, PC_UI_TEXT_BODY);
    if (mapCursor < mapStart)
        mapStart = mapCursor;
    if (mapCursor >= mapStart + 6)
        mapStart = mapCursor - 5;
    for (u32 row = 0; row < 6 && mapStart + row < mapCount; row++) {
        TownMapNativePlace place;
        u32 index = mapStart + row;
        TownMap_GetNativePlace(mapPlaces[index], &place);
        PcUi_Button(font, place.name, (PcUiRect){ 902, 224 + row * 42, 322, 36 }, index == mapCursor,
                    (PcUiAction){ state->ready ? PC_UI_ACTION_ROW : PC_UI_ACTION_NONE, index });
    }
    if (state->selected < state->count) {
        TownMapNativePlace place;
        TownMap_GetNativePlace(state->selected, &place);
        PcUi_RoleParagraph(font, place.description, 906, 493, 1220, 596, PC_UI_WHITE, PC_UI_TEXT_STAT_VALUE);
        PcUi_LineLabel(font, place.canFly ? "Z: Fly here" : "Not available for Fly", 906, 603, 1220,
                       place.canFly ? PC_UI_CYAN : PC_UI_MUTED, PC_UI_TEXT_STAT_VALUE);
    }
}

static void Sync(const TownMapNativeState *state) {
    PcUiInput_Sync(&input, state->font, state->mode | (state->ready << 8));
}
void PcMenuMap_Reset(void) {
    input = (PcUiInput){ 0 };
    mapCount = mapCursor = mapStart = 0;
}
bool PcMenuMap_Draw(void) {
    TownMapNativeState state;
    if (!TownMap_GetNativeState(&state))
        return false;
    Sync(&state);
    PcUi_BeginControls(input.owner, input.phase);
    DrawMap(&state);
    return true;
}
bool PcMenuMap_HandleEvent(PcVideo *video, const SDL_Event *event) {
    TownMapNativeState state;
    if (!TownMap_GetNativeState(&state))
        return false;
    Sync(&state);
    return PcUiInput_Event(&input, video, event);
}
Uint16 PcMenuMap_FilterKeys(Uint16 keys) {
    TownMapNativeState state;
    if (!TownMap_GetNativeState(&state))
        return keys;
    Sync(&state);
    Uint16 pressed;
    PcUiAction action;
    keys = PcUiInput_Read(&input, keys, &pressed, &action);
    RefreshMapPlaces(&state);
    int row = action.kind == PC_UI_ACTION_ROW ? action.value : -1;
    if (mapCount && state.ready) {
        if (pressed & (PAD_KEY_DOWN | PAD_KEY_RIGHT))
            row = (mapCursor + 1) % mapCount;
        if (pressed & (PAD_KEY_UP | PAD_KEY_LEFT))
            row = (mapCursor + mapCount - 1) % mapCount;
        if (row >= 0 && (u32)row < mapCount)
            TownMap_SetNativePlace(mapPlaces[row]);
    }
    return keys & ~(PAD_PLUS_KEY_MASK | PAD_BUTTON_SELECT);
}
