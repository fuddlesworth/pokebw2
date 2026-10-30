#include "../gx_backend.h"
#include <string.h>

void PcGx_InitVRAM(u32 preservedBanks) {
    if (preservedBanks != 0) {
        PcGx.failed = true;
        SDL_strlcpy(PcGx.error, "Partial VRAM-bank clear is not supported", sizeof(PcGx.error));
        return;
    }
    memset(PcGxBackground, 0, sizeof(PcGxBackground));
    memset(PcGxLcdc, 0, sizeof(PcGxLcdc));
    PcGxCapturedBanks = 0;
    PcGxWideCaptureValid = FALSE;
    memset(PcGx2D, 0, sizeof(PcGx2D));
    memset(PcGx.textures, 0, sizeof(PcGx.textures));
    memset(PcGx.palettes, 0, sizeof(PcGx.palettes));
    for (unsigned engine = 0; engine < 2; engine++)
        MIi_CpuClear32(0xc0, PcGx2D[engine].oam, sizeof(PcGx2D[engine].oam));
}

// Preserve the SDK's register-selected VRAM destinations. The original BG address calculations run on the host.
static void Upload(unsigned engine, void *destination, const void *source, u32 offset, u32 size) {
    uptr address = (uptr)destination, base = (uptr)PcGxBackground[engine];
    size_t capacity = engine ? HW_DB_BG_VRAM_SIZE : HW_BG_VRAM_SIZE;
    if (address < base || address - base > capacity || offset > capacity - (address - base) ||
        size > capacity - (address - base) - offset) {
        PcGx.failed = true;
        SDL_strlcpy(PcGx.error, "Background VRAM upload out of bounds", sizeof(PcGx.error));
        return;
    }
    memcpy((u8 *)destination + offset, source, size);
}

#define BG_UPLOAD(prefix, getter, engine, layer, kind)                                                                 \
    void prefix##_LoadBG##layer##kind(const void *source, u32 offset, u32 size) {                                      \
        Upload(engine, getter##_GetBG##layer##kind##Ptr(), source, offset, size);                                      \
    }
BG_UPLOAD(GX, G2, 0, 0, Scr)
BG_UPLOAD(GX, G2, 0, 1, Scr)
BG_UPLOAD(GX, G2, 0, 2, Scr)
BG_UPLOAD(GX, G2, 0, 3, Scr)
BG_UPLOAD(GXS, G2S, 1, 0, Scr)
BG_UPLOAD(GXS, G2S, 1, 1, Scr)
BG_UPLOAD(GXS, G2S, 1, 2, Scr)
BG_UPLOAD(GXS, G2S, 1, 3, Scr)
BG_UPLOAD(GX, G2, 0, 0, Char)
BG_UPLOAD(GX, G2, 0, 1, Char)
BG_UPLOAD(GX, G2, 0, 2, Char)
BG_UPLOAD(GX, G2, 0, 3, Char)
BG_UPLOAD(GXS, G2S, 1, 0, Char)
BG_UPLOAD(GXS, G2S, 1, 1, Char)
BG_UPLOAD(GXS, G2S, 1, 2, Char)
BG_UPLOAD(GXS, G2S, 1, 3, Char)
#undef BG_UPLOAD

#define PALETTE_UPLOAD(prefix, engine, name, member)                                                                   \
    void prefix##_Load##name(const void *source, u32 offset, u32 size) {                                               \
        if (!size)                                                                                                     \
            return;                                                                                                    \
        if (offset > sizeof(PcGx2D[engine].member) || size > sizeof(PcGx2D[engine].member) - offset) {                 \
            PcGx.failed = true;                                                                                        \
            SDL_strlcpy(PcGx.error, "Background palette upload out of bounds", sizeof(PcGx.error));                    \
            return;                                                                                                    \
        }                                                                                                              \
        memcpy(PcGx2D[engine].member + offset, source, size);                                                          \
    }
PALETTE_UPLOAD(GX, 0, BGPltt, background_palette)
PALETTE_UPLOAD(GXS, 1, BGPltt, background_palette)
PALETTE_UPLOAD(GX, 0, BGExtPltt, background_extended_palette)
PALETTE_UPLOAD(GXS, 1, BGExtPltt, background_extended_palette)
#undef PALETTE_UPLOAD

void GX_BeginLoadBGExtPltt(void) {
}
void GX_EndLoadBGExtPltt(void) {
}
void GXS_BeginLoadBGExtPltt(void) {
}
void GXS_EndLoadBGExtPltt(void) {
}

void PcGx_DrawBackgrounds(unsigned engine) {
    PcGx_DrawBackgroundsInViewport(engine, (PcViewport){ 0, 0, PC_CANVAS_WIDTH, PC_CANVAS_HEIGHT }, PC_CANVAS_WIDTH,
                                   PC_CANVAS_HEIGHT);
}

void PcGx_DrawBackgroundsInViewport(unsigned engine, PcViewport viewport, unsigned source_width,
                                    unsigned source_height) {
    PcGx_DrawBackgroundLayers(engine, viewport, source_width, source_height, 31);
}

void PcGx_DrawBackgroundLayers(unsigned engine, PcViewport viewport, unsigned source_width, unsigned source_height,
                               unsigned layers) {
    PcGx_DrawBackgroundRegion(engine, viewport, (SDL_FRect){ 0, 0, source_width, source_height }, layers);
}

void PcGx_DrawBackgroundRegion(unsigned engine, PcViewport viewport, SDL_FRect source, unsigned layers) {
    if (engine > 1)
        return;
    u32 display = engine ? PcGxSubDisplay : PcGxMainDisplay;
    for (unsigned layer = 0; layer < 5; layer++) {
        if (!(layers & (1u << layer)))
            continue;
        if (layer < 4 && (!(display & (1u << (8 + layer))) || (!engine && layer == 0 && (display & 8))))
            continue;
        if (PcGx.vertex_count + 6 > PC_GX_MAX_VERTICES) {
            PcGx.failed = true;
            SDL_strlcpy(PcGx.error, "Background geometry capacity exceeded", sizeof(PcGx.error));
            return;
        }
        unsigned priority = layer == 4 ? 4 : PcGxRegisters[engine].halves[4 + layer] & 3;
        const unsigned corners[] = { 0, 1, 2, 0, 2, 3 };
        for (unsigned i = 0; i < 6; i++) {
            unsigned corner = corners[i];
            float x = corner == 1 || corner == 2 ? 1 : 0;
            float y = corner >= 2 ? 1 : 0;
            PcGxVertex *vertex = &PcGx.vertices[PcGx.vertex_count++];
            *vertex = (PcGxVertex){ .clip = { (viewport.x + x * viewport.width) * 2 / PC_CANVAS_WIDTH - 1,
                                              1 - (viewport.y + y * viewport.height) * 2 / PC_CANVAS_HEIGHT,
                                              0.5f + (priority * 4 + layer) * 0.02f, 1 },
                                    .uv = { source.x + x * source.w, source.y + y * source.h },
                                    .color = { 1, 1, 1, 1 },
                                    .texture = layer,
                                    .palette = engine,
                                    .polygon = 0x80000000u | (31 << 16) | 192 };
        }
    }
}
