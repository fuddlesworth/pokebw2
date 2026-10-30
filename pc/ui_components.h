#ifndef POKEBW2_PC_UI_COMPONENTS_H
#define POKEBW2_PC_UI_COMPONENTS_H

#include "menu_skin.h"
#include "ui_input.h"
#include "ui_widgets.h"
#include "video.h"

void PcUi_BeginControls(const void *owner, unsigned phase);
void PcUi_Control(PcUiShape shape, PcUiAction action);
PcUiAction PcUi_PointerAction(PcVideo *video, const SDL_Event *event, const void *owner, unsigned phase);
PcUiShape PcUi_SkinShape(PcMenuSkinAsset asset, PcUiRect rect);
void PcUi_SkinControl(PcMenuSkinAsset asset, PcUiRect rect, unsigned state, PcUiAction action);
void PcUi_TabLabel(Font *font, const char *label, PcUiShape shape, bool selected);
void PcUi_Tab(Font *font, const char *label, PcUiRect rect, bool selected, PcUiAction action);
void PcUi_Button(Font *font, const u16 *label, PcUiRect rect, bool selected, PcUiAction action);
void PcUi_ButtonLabel(Font *font, const char *label, PcUiRect rect, bool selected, PcUiAction action);

void PcUi_CenteredButtonLabel(Font *font, const char *label, PcUiRect rect, bool selected, bool enabled,
                              PcUiAction action);

#endif
