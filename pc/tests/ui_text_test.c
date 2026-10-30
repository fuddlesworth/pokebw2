#include "../gx_backend.h"
#include "../ui_draw.h"
#include "../ui_widgets.h"
#include "system/gf_font.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// This test owns a CPU-only frame and exercises FreeType glyphs without an original game font archive.
PcGxFrame PcGx;

void GFL_FontGetGlyph(Font *font, u16 code, void *dest, GlyphInfo *info) {
    (void)font;
    (void)code;
    (void)dest;
    (void)info;
    abort();
}

void GFL_TextRndGetGlobalColors(u8 *letter, u8 *shadow, u8 *background) {
    (void)letter;
    (void)shadow;
    (void)background;
    abort();
}

void GFL_TextRndUpdateColorIndexLUT(u8 letter, u8 shadow, u8 background) {
    (void)letter;
    (void)shadow;
    (void)background;
    abort();
}

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "UI text check failed at line %d: %s (%s)\n", __LINE__, #condition, SDL_GetError());       \
            return EXIT_FAILURE;                                                                                       \
        }                                                                                                              \
    } while (0)

static bool Within(float left, float top, float right, float bottom) {
    for (u32 i = 0; i < PcGx.vertex_count; i++) {
        float x = (PcGx.vertices[i].clip[0] + 1) * PC_UI_WIDTH / 2;
        float y = (1 - PcGx.vertices[i].clip[1]) * PC_UI_HEIGHT / 2;
        if (x < left - 0.001f || x > right + 0.001f || y < top - 0.001f || y > bottom + 0.001f)
            return false;
    }
    return true;
}

int main(int argc, char **argv) {
    CHECK(argc == 2 && SDL_Init(0));
    char temporary[512], directory[1024], source[1024], destination[1024];
    SDL_snprintf(temporary, sizeof(temporary), "ui-font-test-%llu/", (unsigned long long)SDL_GetPerformanceCounter());
    SDL_snprintf(directory, sizeof(directory), "%sassets/fonts", temporary);
    CHECK(SDL_CreateDirectory(directory));
    SDL_snprintf(source, sizeof(source), "%sassets/fonts/PixelOperator.ttf", argv[1]);
    SDL_snprintf(destination, sizeof(destination), "%s/PixelOperator.ttf", directory);
    CHECK(SDL_CopyFile(source, destination));
    // The second face is missing: retrying must release the first face, atlas and FreeType library each time.
    for (unsigned i = 0; i < 3; i++) {
        CHECK(!PcUi_LoadFonts(temporary));
        CHECK(PcUi_LabelWidth(NULL, "Text", PC_UI_FONT_BOLD, 1) == 0);
    }
    CHECK(SDL_RemovePath(destination));
    CHECK(SDL_RemovePath(directory));
    SDL_snprintf(directory, sizeof(directory), "%sassets", temporary);
    CHECK(SDL_RemovePath(directory));
    CHECK(SDL_RemovePath(temporary));
    CHECK(PcUi_LoadFonts(argv[1]) && PcUi_Begin());
    const u16 multiline[] = { 'W', '\n', 'W', 'W', 0xffff };
    const u16 commands[] = { 'W', 0xf000, 0xbeef, 2, 'Z', 'Z', 1, '\r', 'W', 0xffff };
    const u16 two[] = { 'W', 'W', 0xffff };
    float pair_width = PcUi_TextWidth(NULL, two, PC_UI_FONT_BOLD, 1);
    CHECK(pair_width > 0);
    CHECK(PcUi_TextWidth(NULL, multiline, PC_UI_FONT_BOLD, 1) == pair_width);
    CHECK(PcUi_LabelWidth(NULL, "W\nWW", PC_UI_FONT_BOLD, 1) == pair_width);
    CHECK(PcUi_TextWidth(NULL, commands, PC_UI_FONT_BOLD, 1) == pair_width);
    PcGx.vertex_count = 0;
    PcUi_Text(NULL, commands, 10, 10, 300, 100, 0xffffff, 1, PC_UI_FONT_BOLD);
    CHECK(PcGx.vertex_count == 12 && Within(10, 10, 300, 26));
    PcGx.vertex_count = 0;
    CHECK(PcUi_Text(NULL, multiline, 10, 10, 300, 100, 0xffffff, 1, PC_UI_FONT_BOLD) == 26);
    CHECK(PcGx.vertex_count == 18 && Within(10, 10, 300, 42));
    // DS line breaks reflow into paragraph spaces; ordinary text above keeps explicit line breaks.
    PcGx.vertex_count = 0;
    PcUi_Paragraph(NULL, multiline, 10, 10, 300, 100, 0xffffff, 1, 20);
    CHECK(PcGx.vertex_count == 18 && Within(10, 10, 300, 26));
    u16 longword[81];
    for (u32 i = 0; i < 80; i++)
        longword[i] = 'W';
    longword[80] = 0xffff;
    PcGx.vertex_count = 0;
    PcUi_Paragraph(NULL, longword, 10, 10, 35, 73, 0xffffff, 1, 20);
    CHECK(PcGx.vertex_count > 18 && PcGx.vertex_count < 80 * 6 && Within(10, 10, 35, 73));
    PcGx.vertex_count = 0;
    PcUi_FittedParagraph(NULL, longword, 10, 10, 35, 73, 0xffffff, 1, 20, PC_UI_FONT_BOLD);
    CHECK(PcGx.vertex_count == 80 * 6 && Within(10, 10, 35, 73));
    for (u32 i = 0; i < PcGx.vertex_count; i += 6)
        CHECK(fabsf(PcGx.vertices[i + 2].uv[1] - PcGx.vertices[i].uv[1] - 16) < 0.001f);
    // This real Black 2/White 2 entry loses its final words at the nominal size in the Pokédex panel.
    const char *entry = "This pitiless Pokémon commands a group\n"
                        "of Pawniard to hound prey into immobility.\n"
                        "It then moves in to finish the prey off.";
    u16 bisharp[256];
    u32 entryLength = 0, entryGlyphs = 0;
    while (*entry) {
        u32 code = SDL_StepUTF8(&entry, NULL);
        bisharp[entryLength++] = (u16)code;
        if (code != ' ' && code != '\n')
            entryGlyphs++;
    }
    bisharp[entryLength] = 0xffff;
    PcGx.vertex_count = 0;
    PcUi_RoleParagraph(NULL, bisharp, 853, 445, 1238, 613, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
    CHECK(PcGx.vertex_count == entryGlyphs * 6 && Within(853, 445, 1238, 613));
    for (u32 i = 0; i < PcGx.vertex_count; i += 6)
        CHECK(fabsf(PcGx.vertices[i + 2].uv[1] - PcGx.vertices[i].uv[1] - 16) < 0.001f);
    // Commands do not consume layout space, and a paragraph that already fits keeps its nominal scale.
    PcGx.vertex_count = 0;
    PcUi_FittedParagraph(NULL, commands, 10, 10, 300, 100, 0xffffff, 1, 17, PC_UI_FONT_BOLD);
    CHECK(PcGx.vertex_count == 12 && Within(10, 10, 300, 26));
    float paragraphHeight = (PcGx.vertices[0].clip[1] - PcGx.vertices[2].clip[1]) * PC_UI_HEIGHT / 2;
    CHECK(fabsf(paragraphHeight - 16) < 0.001f);
    PcGx.vertex_count = 0;
    PcUi_Text(NULL, two, 10, 10, 12, 23, 0xffffff, 1, PC_UI_FONT_BOLD);
    CHECK(PcGx.vertex_count > 0 && Within(10, 10, 12, 23));
    PcGx.vertex_count = 0;
    PcUi_FittedText(NULL, longword, 10, 10, 100, 26, 0xffffff, 1, PC_UI_FONT_BOLD);
    CHECK(PcGx.vertex_count == 80 * 6 && Within(10, 10, 100, 26));
    PcGx.vertex_count = 0;
    PcUi_CenteredLabel(NULL, "WW", 10, 10, 100, 26, 0xffffff, 1);
    CHECK(PcGx.vertex_count == 12 && Within(10, 10, 100, 26));
    float first = (PcGx.vertices[0].clip[0] + 1) * PC_UI_WIDTH / 2;
    float last = (PcGx.vertices[8].clip[0] + 1) * PC_UI_WIDTH / 2;
    CHECK(fabsf((first + last) / 2 - 55) < 0.001f);
    char longlabel[301];
    SDL_memset(longlabel, 'W', 300);
    longlabel[300] = 0;
    float capped_width = PcUi_LabelWidth(NULL, longlabel, PC_UI_FONT_BOLD, 1);
    longlabel[256] = 0;
    CHECK(capped_width == PcUi_LabelWidth(NULL, longlabel, PC_UI_FONT_BOLD, 1));
    longlabel[256] = 'W';
    PcGx.vertex_count = 0;
    PcUi_FittedLabel(NULL, longlabel, 10, 10, 1200, 26, 0xffffff, 1, PC_UI_FONT_BOLD);
    CHECK(PcGx.vertex_count == 256 * 6 && Within(10, 10, 1200, 26));
    CHECK(PcUi_LabelWidth(NULL, "\xc3\xa9", PC_UI_FONT_BOLD, 1) > 0);
    // Every semantic role fits long localized labels into the component's bounds.
    for (PcUiTextRole role = 0; role < PC_UI_TEXT_ROLE_COUNT; role++) {
        const PcUiTextStyle *style = PcUi_Style(role);
        CHECK(style->font != PC_UI_FONT_REGULAR && style->scale > 0);
        PcGx.vertex_count = 0;
        PcUi_RoleLabel(NULL, "A very long label that cannot fit at its preferred size", 10, 10, 120, 34,
                       PC_UI_DEFAULT_COLOR, role);
        CHECK(PcGx.vertex_count && Within(10, 10, 120, 34));
        PcGx.vertex_count = 0;
        PcUi_RoleCenteredLabel(NULL, "WW", 10, 10, 120, 34, PC_UI_DEFAULT_COLOR, role);
        CHECK(PcGx.vertex_count && Within(10, 10, 120, 34));
        PcGx.vertex_count = 0;
        PcUi_RightLabel(NULL, "WW", 10, 10, 120, 34, PC_UI_DEFAULT_COLOR, role);
        CHECK(PcGx.vertex_count && Within(10, 10, 120, 34));
    }
    // Paragraphs honor both the role's face and its horizontal metrics, including condensed headings.
    const u16 spaced[] = { 'W', ' ', 'W', 0xffff };
    const PcUiTextRole paragraphRoles[] = { PC_UI_TEXT_BODY, PC_UI_TEXT_SCREEN_TITLE };
    for (u32 r = 0; r < SDL_arraysize(paragraphRoles); r++) {
        PcUiTextRole role = paragraphRoles[r];
        const PcUiTextStyle *style = PcUi_Style(role);
        PcGx.vertex_count = 0;
        PcUi_RoleText(NULL, spaced, 10, 10, 300, 100, PC_UI_DEFAULT_COLOR, role);
        CHECK(PcGx.vertex_count == 12);
        PcGxVertex reference[12];
        SDL_memcpy(reference, PcGx.vertices, sizeof(reference));
        PcGx.vertex_count = 0;
        PcUi_RoleParagraph(NULL, spaced, 10, 10, 300, 100, PC_UI_DEFAULT_COLOR, role);
        CHECK(PcGx.vertex_count == 12 && Within(10, 10, 300, 100));
        CHECK((PcGx.vertices[0].uv[1] >= 128) == (style->font == PC_UI_FONT_HEADING));
        for (u32 i = 0; i < 12; i++) {
            CHECK(fabsf(PcGx.vertices[i].clip[0] - reference[i].clip[0]) < 0.000001f);
            CHECK(fabsf(PcGx.vertices[i].clip[1] - reference[i].clip[1]) < 0.000001f);
            CHECK(PcGx.vertices[i].uv[0] == reference[i].uv[0]);
            CHECK(PcGx.vertices[i].uv[1] == reference[i].uv[1]);
        }
    }
    PcGx.vertex_count = 0;
    PcUi_FittedParagraph(NULL, two, 10, 10, 300, 100, 0xffffff, 1, 17, PC_UI_FONT_COUNT);
    PcUi_FittedParagraph(NULL, two, 10, 10, 300, 100, 0xffffff, 1, 17, (PcUiFont)-1);
    CHECK(PcGx.vertex_count == 0);
    PcGx.vertex_count = 0;
    PcUi_StatRow(NULL, "Sp. Defense", "999 / 999", (PcUiRect){ 10, 10, 190, 34 }, true);
    CHECK(PcGx.vertex_count && Within(10, 10, 200, 44));
    PcUi_Shutdown();
    PcUi_Shutdown();
    CHECK(PcUi_LabelWidth(NULL, "WW", PC_UI_FONT_BOLD, 1) == 0);
    for (u32 i = PC_UI_ATLAS_BASE; i < sizeof(PcGx.textures); i++)
        CHECK(PcGx.textures[i] == 0);
    CHECK(PcUi_LoadFonts(argv[1]) && PcUi_Begin());
    CHECK(PcUi_TextWidth(NULL, two, PC_UI_FONT_BOLD, 1) == pair_width);
    PcUi_Shutdown();
    SDL_Quit();
    puts("Font failure recovery, cleanup, wrapping, metrics and bounded text passed");
    return EXIT_SUCCESS;
}
