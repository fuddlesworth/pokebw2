#ifndef POKEBW2_PC_UI_FRAME_H
#define POKEBW2_PC_UI_FRAME_H

#include "menu_skin.h"
#include "ui_draw.h"

void PcUiFrame_Header(Font *font, const char *title, const char *section);
void PcUiFrame_Footer(Font *font, const char *hint, const char *action, bool browse, bool enabled);
void PcUiFrame_Draw(Font *font, PcMenuSkinAsset body, const char *title, const char *section, const char *hint);
float PcUiFrame_KeyHint(Font *font, const char *key, const char *label, float right, float centerY, float keyWidth,
                        u16 button);

float PcUiFrame_DirectionHint(Font *font, const char *label, float right, float centerY);

#endif
