#ifndef POKEBW2_PC_MENU_UI_H
#define POKEBW2_PC_MENU_UI_H
#include "video.h"
bool PcMenuUi_IsActive(void);
bool PcMenuUi_Draw(void);
bool PcMenuUi_HandleEvent(PcVideo *video, const SDL_Event *event);
Uint16 PcMenuUi_FilterKeys(Uint16 keys);
#endif
