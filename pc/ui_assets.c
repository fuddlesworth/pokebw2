#include "ui_assets.h"
#include "app/pokedex_info.h"
#include "battle/btlv_gauge.h"
#include "constants/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gx_backend.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "system/app_menu_common.h"
#include "system/poke_icon.h"
#include "ui_draw.h"
#include <SDL3/SDL.h>

static struct {
    u16 pixels[64 * 32];
    u32 type, language, width, height, left, top, visibleWidth, visibleHeight;
    bool loaded;
} typeBadges[8];

static void TypeBadge(u32 type, u32 slot, u32 language, float x, float y, float width, float height, bool centered) {
    if (type >= 18 || slot >= SDL_arraysize(typeBadges))
        return;
    if (!typeBadges[slot].loaded || typeBadges[slot].type != type || typeBadges[slot].language != language) {
        NNSG2dCharacterData *chars;
        NNSG2dPaletteData *palette;
        void *charFile =
            GFL_G2DIOReadOBJNCGR(getUINarcIdx(), AppMenuCommon_GetTypeIconCharFile(type), FALSE, &chars, HEAPID_USER);
        void *paletteFile =
            GFL_G2DIOReadNCLR(getUINarcIdx(), AppMenuCommon_GetTypeIconPaletteFile(), &palette, HEAPID_USER);
        SDL_zero(typeBadges[slot]);
        typeBadges[slot].type = type;
        typeBadges[slot].language = language;
        typeBadges[slot].loaded = true;
        typeBadges[slot].left = 64;
        typeBadges[slot].top = 32;
        if (charFile && paletteFile && chars->szByte >= 256) {
            if (language < 6)
                PokedexInfo_LoadNativeTypeCharacters(type, language, chars->pRawData);
            const u8 *tiles = chars->pRawData;
            const u16 *colors = palette->pRawData;
            u32 paletteBase = AppMenuCommon_GetTypeIconPaletteIndex(type) * 16;
            u32 tilesWide = chars->W == 0xffff ? 4 : chars->W;
            u32 tilesHigh = chars->H == 0xffff ? 2 : chars->H;
            if (tilesWide > 8 || tilesHigh > 4 || tilesWide * tilesHigh * 32 > chars->szByte)
                tilesWide = tilesHigh = 0;
            typeBadges[slot].width = tilesWide * 8;
            typeBadges[slot].height = tilesHigh * 8;
            for (u32 y = 0; y < typeBadges[slot].height; y++) {
                for (u32 x = 0; x < typeBadges[slot].width; x++) {
                    u32 tile = (y / 8) * tilesWide + x / 8;
                    u32 offset = tile * 32 + (y & 7) * 4 + (x & 7) / 2;
                    u32 index = (tiles[offset] >> ((x & 1) * 4)) & 15;
                    typeBadges[slot].pixels[y * 64 + x] = index ? colors[paletteBase + index] | 0x8000 : 0;
                    if (index) {
                        typeBadges[slot].left = SDL_min(typeBadges[slot].left, x);
                        typeBadges[slot].top = SDL_min(typeBadges[slot].top, y);
                        typeBadges[slot].visibleWidth = SDL_max(typeBadges[slot].visibleWidth, x + 1);
                        typeBadges[slot].visibleHeight = SDL_max(typeBadges[slot].visibleHeight, y + 1);
                    }
                }
            }
            if (typeBadges[slot].visibleWidth && typeBadges[slot].visibleHeight) {
                typeBadges[slot].visibleWidth -= typeBadges[slot].left;
                typeBadges[slot].visibleHeight -= typeBadges[slot].top;
            }
        }
        if (paletteFile)
            GFL_HeapFree(paletteFile);
        if (charFile)
            GFL_HeapFree(charFile);
    }
    if (!typeBadges[slot].visibleWidth || !typeBadges[slot].visibleHeight)
        return;
    u16 *atlas = (u16 *)(PcGx.textures + PC_UI_ATLAS_BASE);
    for (u32 y = 0; y < typeBadges[slot].height; y++)
        SDL_memcpy(atlas + (224 - (slot / 4) * 32 + y) * PC_UI_ATLAS_WIDTH + (slot % 4) * 64,
                   typeBadges[slot].pixels + y * 64, typeBadges[slot].width * sizeof(u16));
    float scale = SDL_min(width / typeBadges[slot].visibleWidth, height / typeBadges[slot].visibleHeight);
    if (centered) {
        x += (width - typeBadges[slot].visibleWidth * scale) * 0.5f;
        y += (height - typeBadges[slot].visibleHeight * scale) * 0.5f;
    }
    PcUi_Quad(x, y, typeBadges[slot].visibleWidth * scale, typeBadges[slot].visibleHeight * scale, -0.45f, 1, 1, 1,
              PcUi_FontTexture(), (slot % 4) * 64 + typeBadges[slot].left, 224 - (slot / 4) * 32 + typeBadges[slot].top,
              typeBadges[slot].visibleWidth, typeBadges[slot].visibleHeight);
}

void PcUi_TypeBadge(u32 type, u32 slot, u32 language, float x, float y, float width, float height) {
    TypeBadge(type, slot, language, x, y, width, height, false);
}

void PcUi_CenteredTypeBadge(u32 type, u32 slot, u32 language, float x, float y, float width, float height) {
    TypeBadge(type, slot, language, x, y, width, height, true);
}

void PcUi_BattleGender(u32 sex, float x, float y, float height) {
    static struct {
        u16 pixels[8 * 16];
        u32 left, top, width, height;
    } signs[2];
    static bool loaded;
    if (sex >= SDL_arraysize(signs))
        return;
    if (!loaded) {
        NNSG2dCharacterData *chars;
        NNSG2dPaletteData *palette;
        void *charFile = GFL_G2DIOReadOBJNCGR(ARCID_BTLV, 0x1b2, FALSE, &chars, HEAPID_USER);
        void *paletteFile = GFL_G2DIOReadNCLR(ARCID_BTLV, BtlvGauge_PaletteFile(), &palette, HEAPID_USER);
        if (charFile && paletteFile && chars->szByte >= 0x400 && palette->szByte >= 32) {
            const u8 *tiles = chars->pRawData;
            const u16 *colors = palette->pRawData;
            for (u32 i = 0; i < SDL_arraysize(signs); i++) {
                signs[i].left = 8;
                signs[i].top = 16;
                for (u32 py = 0; py < 16; py++) {
                    for (u32 px = 0; px < 8; px++) {
                        // BtlvGauge_DrawSex takes two consecutive tiles per mark from this sheet.
                        u32 offset = 0x380 + i * 64 + (py / 8) * 32 + (py & 7) * 4 + px / 2;
                        u32 index = (tiles[offset] >> ((px & 1) * 4)) & 15;
                        signs[i].pixels[py * 8 + px] = index ? colors[index] | 0x8000 : 0;
                        if (index) {
                            signs[i].left = SDL_min(signs[i].left, px);
                            signs[i].top = SDL_min(signs[i].top, py);
                            signs[i].width = SDL_max(signs[i].width, px + 1);
                            signs[i].height = SDL_max(signs[i].height, py + 1);
                        }
                    }
                }
                if (signs[i].width && signs[i].height) {
                    signs[i].width -= signs[i].left;
                    signs[i].height -= signs[i].top;
                }
            }
        }
        if (charFile)
            GFL_HeapFree(charFile);
        if (paletteFile)
            GFL_HeapFree(paletteFile);
        loaded = true;
    }
    if (!signs[sex].width || !signs[sex].height)
        return;
    u16 *atlas = (u16 *)(PcGx.textures + PC_UI_ATLAS_BASE);
    for (u32 row = 0; row < 16; row++)
        SDL_memcpy(atlas + (160 + row) * PC_UI_ATLAS_WIDTH + sex * 8, signs[sex].pixels + row * 8, 16);
    float scale = height / signs[sex].height;
    PcUi_Quad(x, y, signs[sex].width * scale, height, -0.45f, 1, 1, 1, PcUi_FontTexture(), sex * 8 + signs[sex].left,
              160 + signs[sex].top, signs[sex].width, signs[sex].height);
}

static void ArchiveIcon(u32 arc, u32 charId, u32 paletteId, u32 paletteIndex, u32 slot, float x, float y, float size) {
    static struct {
        u32 arc, charId, paletteId, paletteIndex;
        bool loaded;
        u16 pixels[32 * 32];
    } icons[8];
    if (slot >= SDL_arraysize(icons))
        return;
    if (!icons[slot].loaded || icons[slot].arc != arc || icons[slot].charId != charId ||
        icons[slot].paletteId != paletteId || icons[slot].paletteIndex != paletteIndex) {
        NNSG2dCharacterData *chars;
        NNSG2dPaletteData *palette;
        void *charFile = GFL_G2DIOReadOBJNCGR(arc, charId, FALSE, &chars, HEAPID_USER);
        void *paletteFile = GFL_G2DIOReadNCLR(arc, paletteId, &palette, HEAPID_USER);
        SDL_zero(icons[slot]);
        icons[slot].arc = arc;
        icons[slot].charId = charId;
        icons[slot].paletteId = paletteId;
        icons[slot].paletteIndex = paletteIndex;
        icons[slot].loaded = true;
        if (charFile && paletteFile && chars->szByte >= 512 && palette->szByte >= (paletteIndex + 1) * 32) {
            const u8 *tiles = chars->pRawData;
            const u16 *colors = (const u16 *)palette->pRawData + paletteIndex * 16;
            for (u32 py = 0; py < 32; py++) {
                for (u32 px = 0; px < 32; px++) {
                    u32 offset = ((py / 8) * 4 + px / 8) * 32 + (py & 7) * 4 + (px & 7) / 2;
                    u32 index = (tiles[offset] >> ((px & 1) * 4)) & 15;
                    icons[slot].pixels[py * 32 + px] = index ? colors[index] | 0x8000 : 0;
                }
            }
        }
        if (charFile)
            GFL_HeapFree(charFile);
        if (paletteFile)
            GFL_HeapFree(paletteFile);
    }
    u16 *atlas = (u16 *)(PcGx.textures + PC_UI_ATLAS_BASE);
    for (u32 row = 0; row < 32; row++)
        SDL_memcpy(atlas + (128 + row) * PC_UI_ATLAS_WIDTH + slot * 32, icons[slot].pixels + row * 32, 64);
    PcUi_Quad(x, y, size, size, -0.45f, 1, 1, 1, PcUi_FontTexture(), slot * 32, 128, 32, 32);
}

void PcUi_ItemIcon(u32 item, u32 slot, float x, float y, float size) {
    if (item)
        ArchiveIcon(PML_ItemGetIconArcID(), GetItemGraphicsDatID(item, ITEM_FILE_ICON_CHAR),
                    GetItemGraphicsDatID(item, ITEM_FILE_ICON_PLTT), 0, slot, x, y, size);
}

void PcUi_RibbonIcon(u32 chars, u32 palette, u32 slot, float x, float y, float size) {
    ArchiveIcon(ARCID_P_STATUS, chars, 8, palette, slot, x, y, size);
}

void PcUi_PokemonIcon(PartyPkm *pkm, u32 slot, float x, float y, float size) {
    BoxPkm *box = PokeParty_GetBoxPkm(pkm);
    ArchiveIcon(ARCID_POKEICON, PokeIcon_GetFileOfPkm(box), PokeIcon_GetPaletteFile(), PokeIcon_GetPaletteOfPkm(box),
                slot, x, y, size);
}
