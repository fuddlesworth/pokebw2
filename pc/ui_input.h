#ifndef POKEBW2_PC_UI_INPUT_H
#define POKEBW2_PC_UI_INPUT_H

#include "ui_layout.h"
#include "video.h"

typedef struct {
    const void *owner;
    unsigned phase;
    Uint16 pendingKeys, previousKeys;
    PcUiAction action;
} PcUiInput;
void PcUiInput_Sync(PcUiInput *input, const void *owner, unsigned phase);
bool PcUiInput_Event(PcUiInput *input, PcVideo *video, const SDL_Event *event);
Uint16 PcUiInput_Read(PcUiInput *input, Uint16 keys, Uint16 *pressed, PcUiAction *action);
Uint16 PcUi_EventKey(const SDL_Event *event);
Uint16 PcUi_MenuEventKey(const SDL_Event *event);
Uint16 PcUi_KeyboardPad(bool menu);

#endif
