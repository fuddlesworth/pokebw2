#ifndef POKEBW2_PC_SETTINGS_UI_H
#define POKEBW2_PC_SETTINGS_UI_H
#include "video.h"
void PcSettingsUi_Reset(void);
bool PcSettingsUi_Draw(void);
Uint16 PcSettingsUi_FilterKeys(Uint16 keys);
bool PcSettingsUi_HandleEvent(PcVideo *video, const SDL_Event *event);
#endif
