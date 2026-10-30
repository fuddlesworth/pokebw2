#include "dex_media.h"
#include "constants/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "gx_backend.h"
#include "nnsys/g3d.h"
#include "pml/poke_graphic.h"
#include "system/mcss.h"
#include <SDL3/SDL.h>

static MCSSSystem *sprites;
static struct {
    MCSS *sprite;
    u16 species;
    u32 form, sex, rare, personality;
    bool egg, back, measured;
    float left, right, top, bottom;
} pokemon[2];
static bool playing;
static u16 map[256 * 256];
static bool mapReady;
static void *areaCharFile, *areaCellFile, *areaAnimFile;
static NNSG2dCharacterData *areaChars;
static NNSG2dCellDataBank *areaCells;
static NNSG2dAnimBankData *areaAnims;

void PcDexMedia_Free(void) {
    if (sprites) {
        MCSSSys_CloseArc(sprites);
        MCSSSys_Free(sprites);
        sprites = NULL;
    }
    SDL_zero(pokemon);
    playing = false;
    if (areaCharFile)
        GFL_HeapFree(areaCharFile);
    if (areaCellFile)
        GFL_HeapFree(areaCellFile);
    if (areaAnimFile)
        GFL_HeapFree(areaAnimFile);
    areaCharFile = areaCellFile = areaAnimFile = NULL;
}

void PcDexMedia_SetPokemon(unsigned slot, u16 species, u32 form, u32 sex, u32 rare, u32 personality, bool egg,
                           bool back) {
    if (slot >= SDL_arraysize(pokemon))
        return;
    if (pokemon[slot].sprite && pokemon[slot].species == species && pokemon[slot].form == form &&
        pokemon[slot].sex == sex && pokemon[slot].rare == rare && pokemon[slot].personality == personality &&
        pokemon[slot].egg == egg && pokemon[slot].back == back)
        return;
    if (!sprites) {
        sprites = MCSSSys_Create(2, HEAPID_USER);
        MCSSSys_SetTexBase(sprites, 128 * 1024);
        MCSSSys_SetPlttBase(sprites, 64 * 1024);
        MCSSSys_EnableOrtho(sprites);
        MCSSSys_OpenArc(sprites, GetPokemonGraphicsARCID());
    }
    if (pokemon[slot].sprite)
        MCSSSys_Remove(sprites, pokemon[slot].sprite);
    MCSSLoadInfo info;
    SetupPokemonLoaderFSTool(species, form, sex, rare, egg, &info, back);
    if (species == 327)
        MCSSSys_SetSpindaLoadCallback(sprites, personality);
    pokemon[slot].sprite = MCSSSys_Add(sprites, 0, 0, 0, &info);
    pokemon[slot].species = species;
    pokemon[slot].personality = personality;
    pokemon[slot].form = form;
    pokemon[slot].sex = sex;
    pokemon[slot].rare = rare;
    pokemon[slot].egg = egg;
    pokemon[slot].back = back;
    pokemon[slot].measured = false;
    VecFx32 scale = { 16 * FX32_ONE, 16 * FX32_ONE, FX32_ONE };
    MCSS_SetScale(pokemon[slot].sprite, &scale);
    MCSS_SetNoShadow(pokemon[slot].sprite, 1);
    MCSS_EnableOrtho(pokemon[slot].sprite);
    MCSS_PauseAnimation(pokemon[slot].sprite);
    MCSS_SetLoopAnimation(pokemon[slot].sprite);
}

void PcDexMedia_Play(bool play) {
    playing = play;
    for (u32 i = 0; i < SDL_arraysize(pokemon); i++) {
        if (pokemon[i].sprite && pokemon[i].measured) {
            if (play)
                MCSS_ResumeAnimation(pokemon[i].sprite);
            else
                MCSS_PauseAnimation(pokemon[i].sprite);
        }
    }
}

void PcDexMedia_Update(void) {
    if (sprites)
        MCSSSys_Update(sprites);
}

static void MeasurePokemon(unsigned slot, u32 first, u32 end) {
    pokemon[slot].left = pokemon[slot].top = 10000;
    pokemon[slot].right = pokemon[slot].bottom = -10000;
    // MCSS cells include transparent margins. Anchor the visible pixels of the resting pose,
    // then keep that transform while the original animation moves its individual cells.
    for (u32 i = first; i + 2 < end; i += 3) {
        const PcGxVertex *v = &PcGx.vertices[i];
        float du1 = v[1].uv[0] - v[0].uv[0], dv1 = v[1].uv[1] - v[0].uv[1];
        float du2 = v[2].uv[0] - v[0].uv[0], dv2 = v[2].uv[1] - v[0].uv[1];
        float determinant = du1 * dv2 - du2 * dv1;
        if (SDL_fabsf(determinant) < 0.001f || ((v->texture >> 26) & 7) != 3)
            continue;
        u32 width = 8u << ((v->texture >> 20) & 7);
        u32 height = 8u << ((v->texture >> 23) & 7);
        u32 base = (v->texture & 0xffff) * 8;
        int left = SDL_max(0, (int)SDL_floorf(SDL_min(v[0].uv[0], SDL_min(v[1].uv[0], v[2].uv[0]))));
        int top = SDL_max(0, (int)SDL_floorf(SDL_min(v[0].uv[1], SDL_min(v[1].uv[1], v[2].uv[1]))));
        int right = SDL_min((int)width, (int)SDL_ceilf(SDL_max(v[0].uv[0], SDL_max(v[1].uv[0], v[2].uv[0]))));
        int bottom = SDL_min((int)height, (int)SDL_ceilf(SDL_max(v[0].uv[1], SDL_max(v[1].uv[1], v[2].uv[1]))));
        for (int y = top; y < bottom; y++) {
            for (int x = left; x < right; x++) {
                u32 pixel = y * width + x;
                u32 address = base + pixel / 2;
                if (address >= sizeof(PcGx.textures) || !(PcGx.textures[address] >> ((pixel & 1) * 4) & 15))
                    continue;
                float u = x + 0.5f - v[0].uv[0], t = y + 0.5f - v[0].uv[1];
                float b = (u * dv2 - du2 * t) / determinant;
                float c = (du1 * t - u * dv1) / determinant;
                float a = 1 - b - c;
                if (a < -0.001f || b < -0.001f || c < -0.001f)
                    continue;
                float px = (a * v[0].clip[0] / v[0].clip[3] + b * v[1].clip[0] / v[1].clip[3] +
                            c * v[2].clip[0] / v[2].clip[3]) *
                           128;
                float py = -(a * v[0].clip[1] / v[0].clip[3] + b * v[1].clip[1] / v[1].clip[3] +
                             c * v[2].clip[1] / v[2].clip[3]) *
                           96;
                pokemon[slot].left = SDL_min(pokemon[slot].left, px - 0.5f);
                pokemon[slot].right = SDL_max(pokemon[slot].right, px + 0.5f);
                pokemon[slot].top = SDL_min(pokemon[slot].top, py - 0.5f);
                pokemon[slot].bottom = SDL_max(pokemon[slot].bottom, py + 0.5f);
            }
        }
    }
    pokemon[slot].measured = pokemon[slot].right > pokemon[slot].left;
}

bool PcDexMedia_DrawPokemon(unsigned slot, float center, float floor, float width, float height) {
    if (slot >= SDL_arraysize(pokemon) || !pokemon[slot].sprite)
        return false;
    for (u32 i = 0; i < SDL_arraysize(pokemon); i++) {
        if (pokemon[i].sprite) {
            if (i == slot)
                MCSS_Show(pokemon[i].sprite);
            else
                MCSS_Hide(pokemon[i].sprite);
        }
    }
    // Finish the game's queued geometry before measuring only this menu's sprite.
    NNS_G3dGeFlushBuffer();
    NNSG3dGlb savedGlobals = NNS_G3dGlb;
    bool wideScene = PcGx.wide_scene;
    PcGx.wide_scene = false;
    NNS_G3dGlbInit();
    MTX_Identity43_(&NNS_G3dGlb.cameraMtx);
    MTX_Identity44_(&NNS_G3dGlb.projMtx);
    u32 first = PcGx.vertex_count;
    G3_MtxMode(GX_MTXMODE_POSITION_VECTOR);
    G3_Identity();
    MCSSSys_Draw(sprites);
    NNS_G3dGeFlushBuffer();
    NNS_G3dGlb = savedGlobals;
    PcGx.wide_scene = wideScene;
    u32 end = PcGx.vertex_count;
    if (end == first)
        return false;
    if (!pokemon[slot].measured) {
        MeasurePokemon(slot, first, end);
        if (!pokemon[slot].measured) {
            PcGx.vertex_count = first;
            return false;
        }
        PcDexMedia_Play(playing);
    }
    float scale = SDL_min(width / SDL_max(1.0f, pokemon[slot].right - pokemon[slot].left),
                          height / SDL_max(1.0f, pokemon[slot].bottom - pokemon[slot].top));
    float near = 10000, far = -10000;
    for (u32 i = first; i < end; i++) {
        float z = PcGx.vertices[i].clip[2] / PcGx.vertices[i].clip[3];
        near = SDL_min(near, z);
        far = SDL_max(far, z);
    }
    for (u32 i = first; i < end; i++) {
        PcGxVertex *vertex = &PcGx.vertices[i];
        float x = vertex->clip[0] / vertex->clip[3] * 128;
        float y = -vertex->clip[1] / vertex->clip[3] * 96;
        x = center + (x - (pokemon[slot].left + pokemon[slot].right) / 2) * scale;
        y = floor + (y - pokemon[slot].bottom) * scale;
        vertex->clip[0] = x * 2 / 1280 - 1;
        vertex->clip[1] = 1 - y * 2 / 720;
        // Preserve cell ordering with enough depth separation for desktop rasterization.
        float z = vertex->clip[2] / vertex->clip[3];
        vertex->clip[2] = far > near ? -0.42f + (z - near) / (far - near) * 0.02f : -0.41f;
        vertex->clip[3] = 1;
        if (PcGx.vertex_count < PC_GX_MAX_VERTICES) {
            PcGxVertex reflected = *vertex;
            float depth = (floor - y) * 0.35f;
            reflected.clip[1] = 1 - (floor + depth) * 2 / 720;
            reflected.clip[2] += 0.03f;
            reflected.color[0] *= 0.75f;
            reflected.color[1] *= 0.75f;
            reflected.color[2] *= 0.75f;
            reflected.color[3] = SDL_clamp(0.3f * (1 - depth / (height * 0.35f)), 0.0f, 0.3f);
            PcGx.vertices[PcGx.vertex_count++] = reflected;
        }
    }
    return true;
}

const u16 *PcDexMedia_Map(void) {
    if (mapReady)
        return map;
    NNSG2dPaletteData *palette;
    void *paletteFile = GFL_G2DIOReadNCLR(ARCID_TOWNMAP_GRA, 0, &palette, HEAPID_USER);
    if (!paletteFile)
        return NULL;
    SDL_zero(map);
    // The original habitat screen composites the back (7/16) and front (5/17) map layers.
    for (u32 layer = 0; layer < 2; layer++) {
        NNSG2dCharacterData *chars;
        NNSG2dScreenData *screen;
        void *charFile = GFL_G2DIOReadBGNCGR(ARCID_TOWNMAP_GRA, layer ? 5 : 7, FALSE, &chars, HEAPID_USER);
        void *screenFile = GFL_G2DIOReadNSCR(ARCID_TOWNMAP_GRA, layer ? 17 : 16, FALSE, &screen, HEAPID_USER);
        if (charFile && screenFile) {
            const u8 *tiles = chars->pRawData;
            const u16 *entries = (const u16 *)screen->rawData;
            const u16 *colors = palette->pRawData;
            for (u32 y = 0; y < 256; y++) {
                for (u32 x = 0; x < 256; x++) {
                    u32 tileOffset = (y / 8) * (screen->screenWidth / 8) + x / 8;
                    if (tileOffset * 2 >= screen->szByte)
                        continue;
                    u16 tile = entries[tileOffset];
                    u32 sx = tile & 0x400 ? 7 - (x & 7) : x & 7;
                    u32 sy = tile & 0x800 ? 7 - (y & 7) : y & 7;
                    u32 offset = (tile & 1023) * 64 + sy * 8 + sx;
                    if (offset < chars->szByte) {
                        u32 index = tiles[offset];
                        u32 color = (tile >> 12) * 256 + index;
                        if (index && color * 2 < palette->szByte)
                            map[y * 256 + x] = colors[color] | 0x8000;
                    }
                }
            }
        }
        if (screenFile)
            GFL_HeapFree(screenFile);
        if (charFile)
            GFL_HeapFree(charFile);
    }
    GFL_HeapFree(paletteFile);
    mapReady = true;
    return map;
}

void PcDexMedia_HighlightArea(u16 *pixels, u16 animation, int left, int top, u32 color) {
    if (!areaCharFile) {
        areaCharFile = GFL_G2DIOReadOBJNCGR(ARCID_POKEDEX_GRA, 15, FALSE, &areaChars, HEAPID_USER);
        areaCellFile = GFL_G2DIOReadNCER(ARCID_POKEDEX_GRA, 29, FALSE, &areaCells, HEAPID_USER);
        areaAnimFile = GFL_G2DIOReadNANR(ARCID_POKEDEX_GRA, 46, FALSE, &areaAnims, HEAPID_USER);
    }
    if (!areaCharFile || !areaCellFile || !areaAnimFile)
        return;
    const NNSG2dAnimSequenceData *sequence = NNS_G2dGetAnimSequenceByIdx(areaAnims, animation);
    if (!sequence || !sequence->numFrames)
        return;
    u16 index = *(const u16 *)sequence->pAnmFrameArray[0].pContent;
    const NNSG2dCellData *cell = NNS_G2dGetCellDataByIdx(areaCells, index);
    const u8 *tiles = areaChars->pRawData;
    u16 tint = ((color >> 19) & 31) | ((color >> 6) & 0x3e0) | ((color << 7) & 0x7c00);
    // Use the original route/cave masks so the highlight follows the map's actual regions.
    for (u16 i = 0; cell && i < cell->numOAMAttrs; i++) {
        GXOamAttr oam;
        NNS_G2dCopyCellAsOamAttr(cell, i, &oam);
        GXOamShape shape = (GXOamShape)(oam.attr0 | ((u32)oam.attr1 << 16));
        int width = NNS_G2dGetOamSizeX(&shape), height = NNS_G2dGetOamSizeY(&shape);
        int ox = oam.attr1 & 511, oy = oam.attr0 & 255;
        if (ox >= 256)
            ox -= 512;
        if (oy >= 128)
            oy -= 256;
        u32 base = (oam.attr2 & 1023) * (32u << areaCells->mappingMode);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int dx = left + ox + x, dy = top + oy + y;
                if (dx < 0 || dx >= 256 || dy < 0 || dy >= 168)
                    continue;
                int sx = oam.attr1 & 0x1000 ? width - 1 - x : x;
                int sy = oam.attr1 & 0x2000 ? height - 1 - y : y;
                u32 offset = base + ((sy / 8) * (width / 8) + sx / 8) * 32 + (sy & 7) * 4 + (sx & 7) / 2;
                if (offset < areaChars->szByte && ((tiles[offset] >> ((sx & 1) * 4)) & 15)) {
                    u16 baseColor = pixels[dy * 256 + dx];
                    u16 red = ((baseColor & 31) * 3 + (tint & 31) * 2) / 5;
                    u16 green = (((baseColor >> 5) & 31) * 3 + ((tint >> 5) & 31) * 2) / 5;
                    u16 blue = (((baseColor >> 10) & 31) * 3 + ((tint >> 10) & 31) * 2) / 5;
                    pixels[dy * 256 + dx] = red | (green << 5) | (blue << 10) | 0x8000;
                }
            }
        }
    }
}
