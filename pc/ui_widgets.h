#ifndef POKEBW2_PC_UI_WIDGETS_H
#define POKEBW2_PC_UI_WIDGETS_H
#include "ui_theme.h"

typedef struct {
    float scale, width;
} PcUiTextFit;
PcUiTextFit PcUi_FitText(Font *font, const u16 *text, PcUiTextRole role, float width);
PcUiTextFit PcUi_FitLabel(Font *font, const char *text, PcUiTextRole role, float width);
void PcUi_RoleLabel(Font *font, const char *text, float x, float y, float right, float bottom, u32 color,
                    PcUiTextRole role);
void PcUi_RoleText(Font *font, const u16 *text, float x, float y, float right, float bottom, u32 color,
                   PcUiTextRole role);
void PcUi_RoleCenteredLabel(Font *font, const char *text, float left, float y, float right, float bottom, u32 color,
                            PcUiTextRole role);
void PcUi_RoleCenteredText(Font *font, const u16 *text, float left, float y, float right, float bottom, u32 color,
                           PcUiTextRole role);
void PcUi_RightLabel(Font *font, const char *text, float left, float y, float right, float bottom, u32 color,
                     PcUiTextRole role);
void PcUi_RoleParagraph(Font *font, const u16 *text, float x, float y, float right, float bottom, u32 color,
                        PcUiTextRole role);
float PcUi_RoleLabelWidth(Font *font, const char *text, PcUiTextRole role);
float PcUi_RoleTextWidth(Font *font, const u16 *text, PcUiTextRole role);
void PcUi_LineLabel(Font *font, const char *text, float x, float y, float right, u32 color, PcUiTextRole role);
void PcUi_LineText(Font *font, const u16 *text, float x, float y, float right, u32 color, PcUiTextRole role);
void PcUi_LineCenteredLabel(Font *font, const char *text, float x, float y, float right, u32 color, PcUiTextRole role);
void PcUi_StatCard(Font *font, const char *label, const char *value, PcUiRect rect);
void PcUi_StatRow(Font *font, const char *label, const char *value, PcUiRect rect, bool lightPanel);
#endif
