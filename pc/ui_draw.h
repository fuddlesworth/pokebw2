#ifndef POKEBW2_PC_UI_DRAW_H
#define POKEBW2_PC_UI_DRAW_H

#include "types.h"
#include "struct_decls.h"
#include <stdbool.h>

enum {
    PC_UI_WIDTH = 1280,
    PC_UI_HEIGHT = 720,
    PC_UI_ATLAS_BASE = 256 * 1024,
    PC_UI_ATLAS_WIDTH = 512,
    PC_UI_ATLAS_HEIGHT = 256
};
typedef enum { PC_UI_FONT_REGULAR, PC_UI_FONT_BOLD, PC_UI_FONT_HEADING, PC_UI_FONT_COUNT } PcUiFont;

// basePath is the asset root, including its trailing directory separator.
bool PcUi_LoadFonts(const char *basePath);
void PcUi_Shutdown(void);
bool PcUi_Begin(void);
u32 PcUi_FontTexture(void);
void PcUi_Quad(float x, float y, float width, float height, float depth, float red, float green, float blue,
               u32 texture, float u, float v, float textureWidth, float textureHeight);
void PcUi_Solid(float x, float y, float width, float height, u32 color, float alpha);
float PcUi_Text(Font *font, const u16 *text, float x, float y, float right, float bottom, u32 color, float scale,
                PcUiFont style);
// Fitted labels stay on one line; explicit newlines become spaces. The scale is an upper bound.
// alignment is 0 for left, 0.5 for centered ink, or 1 for right-aligned ink.
void PcUi_AlignedText(Font *font, const u16 *text, float left, float y, float right, float bottom, u32 color,
                      float scale, PcUiFont style, float alignment);
void PcUi_AlignedLabel(Font *font, const char *text, float left, float y, float right, float bottom, u32 color,
                       float scale, PcUiFont style, float alignment);
void PcUi_FittedText(Font *font, const u16 *text, float x, float y, float right, float bottom, u32 color, float scale,
                     PcUiFont style);
void PcUi_FittedLabel(Font *font, const char *text, float x, float y, float right, float bottom, u32 color, float scale,
                      PcUiFont style);
// Paragraphs reflow DS hard line breaks as spaces, unlike PcUi_Text's explicit newlines.
// Reflow the complete paragraph, reducing scale only if its wrapped glyphs exceed the rectangle.
void PcUi_FittedParagraph(Font *font, const u16 *text, float x, float y, float right, float bottom, u32 color,
                          float scale, float lineHeight, PcUiFont style);
void PcUi_Paragraph(Font *font, const u16 *text, float x, float y, float right, float bottom, u32 color, float scale,
                    float lineHeight);
void PcUi_StyledLabel(Font *font, const char *text, float x, float y, float right, float bottom, u32 color, float scale,
                      PcUiFont style);
void PcUi_Label(Font *font, const char *text, float x, float y, float right, float bottom, u32 color, float scale);
// Single-line metrics flatten explicit line breaks exactly as the fitted widgets do.
float PcUi_SingleLineTextWidth(Font *font, const u16 *text, PcUiFont style, float scale);
float PcUi_SingleLineLabelWidth(Font *font, const char *text, PcUiFont style, float scale);
float PcUi_LabelWidth(Font *font, const char *text, PcUiFont style, float scale);
float PcUi_TextWidth(Font *font, const u16 *text, PcUiFont style, float scale);
void PcUi_CenteredText(Font *font, const u16 *text, float left, float y, float right, float bottom, u32 color,
                       float scale);
void PcUi_CenteredLabel(Font *font, const char *text, float left, float y, float right, float bottom, u32 color,
                        float scale);

#endif
