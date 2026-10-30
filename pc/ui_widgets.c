#include "ui_widgets.h"
#include <SDL3/SDL.h>

static u32 TextColor(const PcUiTextStyle *style, u32 color) {
    return color == PC_UI_DEFAULT_COLOR ? style->color : color;
}

static PcUiTextFit Fit(float measured, PcUiTextRole role, float width) {
    float scale = PcUi_Style(role)->scale;
    if (measured > width && measured > 0)
        scale *= SDL_max(width, 0) / measured;
    return (PcUiTextFit){ scale, SDL_min(measured, SDL_max(width, 0)) };
}
PcUiTextFit PcUi_FitText(Font *font, const u16 *text, PcUiTextRole role, float width) {
    return Fit(PcUi_RoleTextWidth(font, text, role), role, width);
}
PcUiTextFit PcUi_FitLabel(Font *font, const char *text, PcUiTextRole role, float width) {
    return Fit(PcUi_RoleLabelWidth(font, text, role), role, width);
}

void PcUi_RoleLabel(Font *font, const char *text, float x, float y, float right, float bottom, u32 color,
                    PcUiTextRole role) {
    const PcUiTextStyle *style = PcUi_Style(role);
    PcUi_FittedLabel(font, text, x, y, right, bottom, TextColor(style, color), style->scale, style->font);
}
void PcUi_RoleText(Font *font, const u16 *text, float x, float y, float right, float bottom, u32 color,
                   PcUiTextRole role) {
    const PcUiTextStyle *style = PcUi_Style(role);
    PcUi_FittedText(font, text, x, y, right, bottom, TextColor(style, color), style->scale, style->font);
}
void PcUi_RoleCenteredLabel(Font *font, const char *text, float left, float y, float right, float bottom, u32 color,
                            PcUiTextRole role) {
    const PcUiTextStyle *style = PcUi_Style(role);
    PcUi_AlignedLabel(font, text, left, y, right, bottom, TextColor(style, color), style->scale, style->font, 0.5f);
}
void PcUi_RoleCenteredText(Font *font, const u16 *text, float left, float y, float right, float bottom, u32 color,
                           PcUiTextRole role) {
    const PcUiTextStyle *style = PcUi_Style(role);
    PcUi_AlignedText(font, text, left, y, right, bottom, TextColor(style, color), style->scale, style->font, 0.5f);
}
void PcUi_RightLabel(Font *font, const char *text, float left, float y, float right, float bottom, u32 color,
                     PcUiTextRole role) {
    const PcUiTextStyle *style = PcUi_Style(role);
    PcUi_AlignedLabel(font, text, left, y, right, bottom, TextColor(style, color), style->scale, style->font, 1);
}
void PcUi_RoleParagraph(Font *font, const u16 *text, float x, float y, float right, float bottom, u32 color,
                        PcUiTextRole role) {
    const PcUiTextStyle *style = PcUi_Style(role);
    PcUi_FittedParagraph(font, text, x, y, right, bottom, TextColor(style, color), style->scale, style->lineHeight,
                         style->font);
}
float PcUi_RoleLabelWidth(Font *font, const char *text, PcUiTextRole role) {
    const PcUiTextStyle *style = PcUi_Style(role);
    return PcUi_SingleLineLabelWidth(font, text, style->font, style->scale);
}
float PcUi_RoleTextWidth(Font *font, const u16 *text, PcUiTextRole role) {
    const PcUiTextStyle *style = PcUi_Style(role);
    return PcUi_SingleLineTextWidth(font, text, style->font, style->scale);
}
void PcUi_LineLabel(Font *font, const char *text, float x, float y, float right, u32 color, PcUiTextRole role) {
    PcUi_RoleLabel(font, text, x, y, right, y + PcUi_Style(role)->scale * 18, color, role);
}
void PcUi_LineText(Font *font, const u16 *text, float x, float y, float right, u32 color, PcUiTextRole role) {
    PcUi_RoleText(font, text, x, y, right, y + PcUi_Style(role)->scale * 18, color, role);
}
void PcUi_LineCenteredLabel(Font *font, const char *text, float x, float y, float right, u32 color, PcUiTextRole role) {
    PcUi_RoleCenteredLabel(font, text, x, y, right, y + PcUi_Style(role)->scale * 18, color, role);
}
void PcUi_StatRow(Font *font, const char *label, const char *value, PcUiRect rect, bool lightPanel) {
    float split = rect.x + rect.width * 0.65f;
    PcUi_RoleLabel(font, label, rect.x, rect.y, split - PC_UI_SPACE_SM, rect.y + rect.height,
                   lightPanel ? PC_UI_INK_MUTED : PC_UI_MUTED, PC_UI_TEXT_STAT_LABEL);
    PcUi_RightLabel(font, value, split, rect.y, rect.x + rect.width, rect.y + rect.height,
                    lightPanel ? PC_UI_INK : PC_UI_WHITE, PC_UI_TEXT_STAT_VALUE);
}

void PcUi_StatCard(Font *font, const char *label, const char *value, PcUiRect rect) {
    float valueTop = rect.y + PC_UI_SPACE_XL + PC_UI_SPACE_XS / 2;
    PcUi_RoleLabel(font, label, rect.x, rect.y, rect.x + rect.width, valueTop, PC_UI_INK_MUTED, PC_UI_TEXT_LABEL);
    PcUi_RoleLabel(font, value, rect.x, valueTop, rect.x + rect.width, rect.y + rect.height, PC_UI_INK,
                   PC_UI_TEXT_CARD_TITLE);
}
