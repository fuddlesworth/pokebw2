#include "ui_draw.h"
#include "gfl/graphics.h"
#include "gx_backend.h"
#include "system/gf_font.h"
#include <SDL3/SDL.h>
#include <ft2build.h>
#include FT_FREETYPE_H

static u16 fontPixels[PC_UI_ATLAS_WIDTH * PC_UI_ATLAS_HEIGHT];
static bool fontReady;
static FT_Library fontLibrary;
static FT_Face fontFaces[PC_UI_FONT_COUNT];
static struct {
    u16 code;
    u8 advance, left, right;
    u32 used;
    bool loaded;
} glyphs[PC_UI_FONT_COUNT][128];
static u32 glyphFrame;

void PcUi_Quad(float x, float y, float w, float h, float z, float r, float g, float b, u32 texture, float u, float v,
               float tw, float th) {
    static const u8 corners[6] = { 0, 1, 2, 0, 2, 3 };
    if (PcGx.vertex_count + 6 > PC_GX_MAX_VERTICES)
        return;
    for (u32 i = 0; i < 6; i++) {
        u32 corner = corners[i];
        float right = corner == 1 || corner == 2;
        float bottom = corner >= 2;
        PcGx.vertices[PcGx.vertex_count++] = (PcGxVertex){ .clip = { (x + right * w) * 2 / PC_UI_WIDTH - 1,
                                                                     1 - (y + bottom * h) * 2 / PC_UI_HEIGHT, z, 1 },
                                                           .uv = { u + right * tw, v + bottom * th },
                                                           .color = { r, g, b, 1 },
                                                           .texture = texture,
                                                           .palette = 0,
                                                           .polygon = PC_GX_POLYGON_UI | 192 | (31 << 16) };
    }
}

void PcUi_Shutdown(void) {
    for (u32 style = 0; style < PC_UI_FONT_COUNT; style++) {
        if (fontFaces[style])
            FT_Done_Face(fontFaces[style]);
        fontFaces[style] = NULL;
    }
    if (fontLibrary)
        FT_Done_FreeType(fontLibrary);
    fontLibrary = NULL;
    fontReady = false;
    glyphFrame = 0;
    SDL_memset(glyphs, 0, sizeof(glyphs));
    SDL_memset(fontPixels, 0, sizeof(fontPixels));
    SDL_memset(PcGx.textures + PC_UI_ATLAS_BASE, 0, sizeof(fontPixels));
}

static void GlyphBounds(u32 style, u32 slot, const u16 *bitmap, u32 pitch) {
    u8 left = 16, right = 0;
    for (u32 y = 0; y < 16; y++) {
        for (u32 x = 0; x < 16; x++) {
            if (bitmap[y * pitch + x]) {
                left = SDL_min(left, x);
                right = SDL_max(right, x + 1);
            }
        }
    }
    glyphs[style][slot].left = right ? left : 0;
    glyphs[style][slot].right = right;
}

bool PcUi_LoadFonts(const char *basePath) {
    if (!fontReady) {
        char path[1024];
        if (!basePath || FT_Init_FreeType(&fontLibrary)) {
            PcUi_Shutdown();
            return false;
        }
        static const char *const files[] = { "PixelOperator.ttf", "PixelOperator-Bold.ttf", "PixelOperatorHB8.ttf" };
        bool okay = true;
        for (u32 weight = 0; weight < PC_UI_FONT_COUNT && okay; weight++) {
            FT_Face face;
            SDL_snprintf(path, sizeof(path), "%sassets/fonts/%s", basePath, files[weight]);
            if (FT_New_Face(fontLibrary, path, 0, &face)) {
                okay = false;
                break;
            }
            fontFaces[weight] = face;
            okay = FT_Set_Pixel_Sizes(face, 0, 16) == 0;
            for (u32 c = 32; c < 128 && okay; c++) {
                okay = FT_Load_Char(face, c == 127 ? 0xe9 : c, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO) == 0;
                if (!okay)
                    break;
                FT_GlyphSlot glyph = face->glyph;
                glyphs[weight][c].code = c == 127 ? 0xe9 : c;
                glyphs[weight][c].advance = (u8)(glyph->advance.x / 64);
                glyphs[weight][c].loaded = true;
                for (u32 y = 0; y < glyph->bitmap.rows; y++) {
                    for (u32 x = 0; x < glyph->bitmap.width; x++) {
                        int px = glyph->bitmap_left + (int)x;
                        int py = (weight == PC_UI_FONT_HEADING ? 14 : 12) - glyph->bitmap_top + (int)y;
                        if (px >= 0 && px < 16 && py >= 0 && py < 16 &&
                            (glyph->bitmap.buffer[y * glyph->bitmap.pitch + x / 8] & (0x80 >> (x & 7))))
                            fontPixels[((c >> 4) * 16 + py + (weight == PC_UI_FONT_HEADING ? 128 : 0)) *
                                           PC_UI_ATLAS_WIDTH +
                                       (weight ? 256 : 0) + (c & 15) * 16 + px] = 0xffff;
                    }
                }
            }
        }
        if (!okay) {
            PcUi_Shutdown();
            return false;
        }
        for (u32 style = 0; style < PC_UI_FONT_COUNT; style++) {
            for (u32 slot = 32; slot < 128; slot++) {
                u32 tx = (slot & 15) * 16 + (style ? 256 : 0);
                u32 ty = (slot >> 4) * 16 + (style == PC_UI_FONT_HEADING ? 128 : 0);
                GlyphBounds(style, slot, fontPixels + ty * PC_UI_ATLAS_WIDTH + tx, PC_UI_ATLAS_WIDTH);
            }
        }
        glyphFrame = 1;
        fontReady = true;
    }
    return true;
}

bool PcUi_Begin(void) {
    if (!PcUi_LoadFonts(SDL_GetBasePath()))
        return false;
    u16 *atlas = (u16 *)(PcGx.textures + PC_UI_ATLAS_BASE);
    SDL_memset(atlas, 0, PC_UI_ATLAS_WIDTH * PC_UI_ATLAS_HEIGHT * sizeof(*atlas));
    SDL_memcpy(atlas, fontPixels, sizeof(fontPixels));
    atlas[255 * PC_UI_ATLAS_WIDTH + 255] = 0xffff;
    glyphFrame++;
    return true;
}

static u32 Glyph(Font *font, u16 code, PcUiFont style) {
    u32 slot = 0;
    for (u32 i = 0; i < 128; i++) {
        if (glyphs[style][i].loaded && glyphs[style][i].code == code) {
            glyphs[style][i].used = glyphFrame;
            return i;
        }
        if (glyphs[style][i].code != '?' &&
            (!glyphs[style][i].loaded || glyphs[style][i].used < glyphs[style][slot].used))
            slot = i;
    }
    if (glyphs[style][slot].used == glyphFrame)
        return Glyph(font, '?', style);
    u32 tx = (slot & 15) * 16 + (style ? 256 : 0);
    u32 ty = (slot >> 4) * 16 + (style == PC_UI_FONT_HEADING ? 128 : 0);
    u16 bitmap[16 * 16] = { 0 };
    FT_Face face = fontFaces[style];
    if (FT_Get_Char_Index(face, code) && !FT_Load_Char(face, code, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO)) {
        FT_GlyphSlot glyph = face->glyph;
        glyphs[style][slot].advance = glyph->advance.x / 64;
        for (u32 y = 0; y < glyph->bitmap.rows; y++) {
            for (u32 x = 0; x < glyph->bitmap.width; x++) {
                int px = glyph->bitmap_left + (int)x;
                int py = (style == PC_UI_FONT_HEADING ? 14 : 12) - glyph->bitmap_top + (int)y;
                if (px >= 0 && px < 16 && py >= 0 && py < 16 &&
                    (glyph->bitmap.buffer[y * glyph->bitmap.pitch + x / 8] & (0x80 >> (x & 7))))
                    bitmap[py * 16 + px] = 0xffff;
            }
        }
    } else if (font) {
        u8 tiles[128] = { 0 }, letter, shadow, background;
        GlyphInfo info;
        GFL_TextRndGetGlobalColors(&letter, &shadow, &background);
        GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
        GFL_FontGetGlyph(font, code, tiles, &info);
        GFL_TextRndUpdateColorIndexLUT(letter, shadow, background);
        glyphs[style][slot].advance = info.advance;
        for (u32 y = 0; y < 16; y++) {
            for (u32 x = 0; x < 16; x++) {
                u32 offset = ((y / 8) * 2 + x / 8) * 32 + (y & 7) * 4 + (x & 7) / 2;
                if (((tiles[offset] >> ((x & 1) * 4)) & 15) == 1 && x + info.x < 16)
                    bitmap[y * 16 + x + info.x] = 0xffff;
            }
        }
    } else {
        return Glyph(font, '?', style);
    }
    GlyphBounds(style, slot, bitmap, 16);
    glyphs[style][slot].code = code;
    glyphs[style][slot].loaded = true;
    glyphs[style][slot].used = glyphFrame;
    u16 *atlas = (u16 *)(PcGx.textures + PC_UI_ATLAS_BASE);
    for (u32 y = 0; y < 16; y++) {
        SDL_memcpy(fontPixels + (ty + y) * PC_UI_ATLAS_WIDTH + tx, bitmap + y * 16, 32);
        SDL_memcpy(atlas + (ty + y) * PC_UI_ATLAS_WIDTH + tx, bitmap + y * 16, 32);
    }
    return slot;
}

u32 PcUi_FontTexture(void) {
    return (7u << 26) | (5u << 23) | (6u << 20) | (PC_UI_ATLAS_BASE / 8);
}

enum { TEXT_LIMIT = 256 };

static bool NextCharacter(const u16 *text, u32 *index, u16 *code) {
    while (*index < TEXT_LIMIT && text[*index] && text[*index] != 0xffff) {
        u16 c = text[(*index)++];
        if (c == 0xf000) {
            u32 command = *index - 1;
            if (command + 3 > TEXT_LIMIT || text[command + 2] > TEXT_LIMIT - command - 3) {
                *index = TEXT_LIMIT;
                return false;
            }
            // Original string commands have a marker, command ID, argument count, then arguments.
            *index = command + 3 + text[command + 2];
            continue;
        }
        if (c == '\n' || c == 0xe000 || c == 0xfffe) {
            *code = '\n';
            return true;
        }
        if (c == '\t' || c == 0x3000)
            c = ' ';
        if (c < 32 || c >= 0xe000)
            continue;
        *code = c;
        return true;
    }
    return false;
}

static void DecodeLabel(const char *text, u16 wide[TEXT_LIMIT + 1]) {
    u32 count = 0;
    if (text) {
        while (*text && count < TEXT_LIMIT) {
            u32 codepoint = SDL_StepUTF8(&text, NULL);
            wide[count++] = codepoint < 0xe000 || (codepoint > 0xf8ff && codepoint < 0xfffe) ? (u16)codepoint : '?';
        }
    }
    wide[count] = 0xffff;
}

typedef struct {
    float width, inkLeft, inkRight;
    bool hasInk;
} TextMetrics;

static TextMetrics Measure(Font *font, const u16 *text, PcUiFont style, bool singleLine) {
    TextMetrics metrics = { 0 };
    float x = 0;
    u32 index = 0;
    u16 code;
    while (NextCharacter(text, &index, &code)) {
        if (code == '\n') {
            if (!singleLine) {
                x = 0;
                continue;
            }
            code = ' ';
        }
        u32 glyph = Glyph(font, code, style);
        if (glyphs[style][glyph].right) {
            if (!metrics.hasInk)
                metrics.inkLeft = x + glyphs[style][glyph].left;
            metrics.hasInk = true;
            metrics.inkRight = SDL_max(metrics.inkRight, x + glyphs[style][glyph].right);
        }
        metrics.width = SDL_max(metrics.width, x + glyphs[style][glyph].right);
        x += glyphs[style][glyph].advance;
        metrics.width = SDL_max(metrics.width, x);
    }
    return metrics;
}

static void DrawGlyph(u32 glyph, PcUiFont style, float x, float y, float right, float bottom, u32 color, float scale) {
    float horizontal = scale * (style == PC_UI_FONT_HEADING ? 0.8f : 1);
    float left = glyphs[style][glyph].left;
    x += left * horizontal;
    float width = SDL_min((glyphs[style][glyph].right - left) * horizontal, right - x);
    float height = SDL_min(16 * scale, bottom - y);
    if (width <= 0 || height <= 0)
        return;
    PcUi_Quad(x, y, width, height, -0.6f, ((color >> 16) & 255) / 255.0f, ((color >> 8) & 255) / 255.0f,
              (color & 255) / 255.0f, PcUi_FontTexture(), (glyph & 15) * 16 + (style ? 256 : 0) + left,
              (glyph >> 4) * 16 + (style == PC_UI_FONT_HEADING ? 128 : 0), width / horizontal, height / scale);
}

float PcUi_Text(Font *font, const u16 *text, float x, float y, float right, float bottom, u32 color, float scale,
                PcUiFont style) {
    if (!fontReady || !text || scale <= 0 || right <= x || bottom <= y || style >= PC_UI_FONT_COUNT)
        return y;
    float start = x;
    float horizontal = scale * (style == PC_UI_FONT_HEADING ? 0.8f : 1);
    u32 index = 0;
    u16 code;
    while (NextCharacter(text, &index, &code)) {
        if (code == '\n') {
            x = start;
            y += 16 * scale;
            continue;
        }
        u32 glyph = Glyph(font, code, style);
        float advance = glyphs[style][glyph].advance * horizontal;
        float extent = SDL_max(glyphs[style][glyph].advance, glyphs[style][glyph].right) * horizontal;
        if (x > start && x + extent > right) {
            x = start;
            y += 16 * scale;
        }
        if (y + 12 * scale > bottom)
            break;
        DrawGlyph(glyph, style, x, y, right, bottom, color, scale);
        x += advance;
    }
    return y;
}

static bool LayoutParagraph(Font *font, const u16 *text, float x, float y, float right, float bottom, u32 color,
                            float scale, float lineHeight, PcUiFont style, bool draw) {
    bool fits = true;
    float horizontal = scale * (style == PC_UI_FONT_HEADING ? 0.8f : 1);
    float start = x;
    u32 index = 0;
    u16 code;
    while (NextCharacter(text, &index, &code)) {
        // Game strings contain hard wraps for the DS screen; native paragraphs reflow them as spaces.
        if (code == ' ' || code == '\n') {
            x += glyphs[style][Glyph(font, ' ', style)].advance * horizontal;
            continue;
        }
        u32 glyph = Glyph(font, code, style);
        float width = glyphs[style][glyph].advance * horizontal;
        if (code < 0x2e80) {
            u32 lookahead = index;
            u16 next;
            while (NextCharacter(text, &lookahead, &next) && next != ' ' && next != '\n' && next < 0x2e80)
                width += glyphs[style][Glyph(font, next, style)].advance * horizontal;
        }
        if (x > start && x + width > right) {
            x = start;
            y += lineHeight * scale;
        }
        for (;;) {
            glyph = Glyph(font, code, style);
            float advance = glyphs[style][glyph].advance * horizontal;
            float extent = SDL_max(glyphs[style][glyph].advance, glyphs[style][glyph].right) * horizontal;
            if (x > start && x + extent > right) {
                x = start;
                y += lineHeight * scale;
            }
            if (glyphs[style][glyph].right) {
                if (x + glyphs[style][glyph].right * horizontal > right || y + 16 * scale > bottom)
                    fits = false;
                if (draw) {
                    if (y + 12 * scale > bottom)
                        return false;
                    DrawGlyph(glyph, style, x, y, right, bottom, color, scale);
                }
            }
            x += advance;
            u32 nextIndex = index;
            u16 next;
            if (code >= 0x2e80 || !NextCharacter(text, &nextIndex, &next) || next == ' ' || next == '\n' ||
                next >= 0x2e80)
                break;
            index = nextIndex;
            code = next;
        }
    }
    return fits;
}

void PcUi_Paragraph(Font *font, const u16 *text, float x, float y, float right, float bottom, u32 color, float scale,
                    float lineHeight) {
    if (!fontReady || !text || scale <= 0 || lineHeight <= 0 || right <= x || bottom <= y)
        return;
    LayoutParagraph(font, text, x, y, right, bottom, color, scale, lineHeight, PC_UI_FONT_BOLD, true);
}

void PcUi_FittedParagraph(Font *font, const u16 *text, float x, float y, float right, float bottom, u32 color,
                          float scale, float lineHeight, PcUiFont style) {
    if (!fontReady || !text || scale <= 0 || lineHeight <= 0 || right <= x || bottom <= y ||
        (unsigned)style >= PC_UI_FONT_COUNT)
        return;
    if (!LayoutParagraph(font, text, x, y, right, bottom, color, scale, lineHeight, style, false)) {
        float upper = scale;
        // First find a fitting bracket. This also accommodates a single glyph wider than the whole column.
        do {
            scale *= 0.5f;
            if (scale == 0)
                return;
        } while (!LayoutParagraph(font, text, x, y, right, bottom, color, scale, lineHeight, style, false));
        float lower = scale;
        for (unsigned i = 0; i < 20; i++) {
            float candidate = lower + (upper - lower) / 2;
            if (LayoutParagraph(font, text, x, y, right, bottom, color, candidate, lineHeight, style, false))
                lower = candidate;
            else
                upper = candidate;
        }
        scale = lower;
    }
    LayoutParagraph(font, text, x, y, right, bottom, color, scale, lineHeight, style, true);
}

void PcUi_StyledLabel(Font *font, const char *text, float x, float y, float right, float bottom, u32 color, float scale,
                      PcUiFont style) {
    u16 wide[TEXT_LIMIT + 1];
    DecodeLabel(text, wide);
    PcUi_Text(font, wide, x, y, right, bottom, color, scale, style);
}

void PcUi_Label(Font *font, const char *text, float x, float y, float right, float bottom, u32 color, float scale) {
    PcUi_StyledLabel(font, text, x, y, right, bottom, color, scale, PC_UI_FONT_BOLD);
}

float PcUi_TextWidth(Font *font, const u16 *text, PcUiFont style, float scale) {
    if (!fontReady || !text || scale <= 0 || style >= PC_UI_FONT_COUNT)
        return 0;
    return Measure(font, text, style, false).width * scale * (style == PC_UI_FONT_HEADING ? 0.8f : 1);
}

float PcUi_SingleLineTextWidth(Font *font, const u16 *text, PcUiFont style, float scale) {
    if (!fontReady || !text || scale <= 0 || style >= PC_UI_FONT_COUNT)
        return 0;
    return Measure(font, text, style, true).width * scale * (style == PC_UI_FONT_HEADING ? 0.8f : 1);
}
float PcUi_SingleLineLabelWidth(Font *font, const char *text, PcUiFont style, float scale) {
    u16 wide[TEXT_LIMIT + 1];
    DecodeLabel(text, wide);
    return PcUi_SingleLineTextWidth(font, wide, style, scale);
}

float PcUi_LabelWidth(Font *font, const char *text, PcUiFont style, float scale) {
    u16 wide[TEXT_LIMIT + 1];
    DecodeLabel(text, wide);
    return PcUi_TextWidth(font, wide, style, scale);
}

static void FittedText(Font *font, const u16 *text, float left, float y, float right, float bottom, u32 color,
                       float scale, PcUiFont style, float alignment) {
    if (!fontReady || !text || scale <= 0 || right <= left || bottom <= y || style >= PC_UI_FONT_COUNT)
        return;
    TextMetrics metrics = Measure(font, text, style, true);
    float horizontal = style == PC_UI_FONT_HEADING ? 0.8f : 1;
    float width = alignment > 0 && metrics.hasInk ? metrics.inkRight - metrics.inkLeft : metrics.width;
    if (width > 0)
        scale = SDL_min(scale, (right - left) / (width * horizontal));
    scale = SDL_min(scale, (bottom - y) / 16);
    float x = left;
    if (alignment > 0 && metrics.hasInk)
        x += (right - left - width * scale * horizontal) * alignment - metrics.inkLeft * scale * horizontal;
    u32 index = 0;
    u16 code;
    while (NextCharacter(text, &index, &code)) {
        u32 glyph = Glyph(font, code == '\n' ? ' ' : code, style);
        DrawGlyph(glyph, style, x, y, right, bottom, color, scale);
        x += glyphs[style][glyph].advance * scale * horizontal;
    }
}

void PcUi_AlignedText(Font *font, const u16 *text, float left, float y, float right, float bottom, u32 color,
                      float scale, PcUiFont style, float alignment) {
    FittedText(font, text, left, y, right, bottom, color, scale, style, SDL_clamp(alignment, 0, 1));
}
void PcUi_AlignedLabel(Font *font, const char *text, float left, float y, float right, float bottom, u32 color,
                       float scale, PcUiFont style, float alignment) {
    u16 wide[TEXT_LIMIT + 1];
    DecodeLabel(text, wide);
    PcUi_AlignedText(font, wide, left, y, right, bottom, color, scale, style, alignment);
}

void PcUi_FittedText(Font *font, const u16 *text, float x, float y, float right, float bottom, u32 color, float scale,
                     PcUiFont style) {
    FittedText(font, text, x, y, right, bottom, color, scale, style, false);
}

void PcUi_FittedLabel(Font *font, const char *text, float x, float y, float right, float bottom, u32 color, float scale,
                      PcUiFont style) {
    u16 wide[TEXT_LIMIT + 1];
    DecodeLabel(text, wide);
    PcUi_FittedText(font, wide, x, y, right, bottom, color, scale, style);
}

void PcUi_CenteredText(Font *font, const u16 *text, float left, float y, float right, float bottom, u32 color,
                       float scale) {
    FittedText(font, text, left, y, right, bottom, color, scale, PC_UI_FONT_BOLD, 0.5f);
}

void PcUi_CenteredLabel(Font *font, const char *text, float left, float y, float right, float bottom, u32 color,
                        float scale) {
    u16 wide[TEXT_LIMIT + 1];
    DecodeLabel(text, wide);
    PcUi_CenteredText(font, wide, left, y, right, bottom, color, scale);
}

void PcUi_Solid(float x, float y, float width, float height, u32 color, float alpha) {
    u32 first = PcGx.vertex_count;
    PcUi_Quad(x, y, width, height, -0.61f, ((color >> 16) & 255) / 255.0f, ((color >> 8) & 255) / 255.0f,
              (color & 255) / 255.0f, PcUi_FontTexture(), 255, 255, 1, 1);
    for (u32 i = first; i < PcGx.vertex_count; i++)
        PcGx.vertices[i].color[3] = alpha;
}
