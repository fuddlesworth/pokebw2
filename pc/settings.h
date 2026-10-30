#ifndef POKEBW2_PC_SETTINGS_H
#define POKEBW2_PC_SETTINGS_H
#include "host_settings.h"
#include "video.h"

enum { PC_SETTINGS_KEYS = 12, PC_SETTINGS_RECORD_SIZE = 256 };
typedef struct {
    Uint8 fullscreen, resolution, vsync, frameLimit, volume;
    Uint8 gameValid, textSpeed, battleScene, battleStyle, saveBeforeIR, sound;
    Uint16 keys[PC_SETTINGS_KEYS], alternate[PC_SETTINGS_KEYS];
    Uint8 antialiasing, integerScale, controller, deadzone, invertWheel, spriteFilter;
    Uint16 controllerButtons[PC_SETTINGS_KEYS];
    char controllerGuid[33];
    Uint8 leftStick;
    Uint8 controllerSlot;
    Uint64 controllerPathHash;
} PcSettings;
void PcSettings_Defaults(PcSettings *settings);
const PcSettings *PcSettings_Get(void);
void PcSettings_Load(void);
bool PcSettings_Save(const PcSettings *settings);
bool PcSettings_ApplyVideo(PcVideo *video, const PcSettings *settings);
void PcSettings_LimitFrame(Uint64 start);
Uint16 PcSettings_Key(SDL_Scancode code);
Uint16 PcSettings_Keyboard(void);
Uint16 PcSettings_Gamepad(void);
Uint16 PcSettings_MenuGamepad(void);
SDL_Gamepad *PcSettings_Controller(void);
void PcSettings_CycleController(PcSettings *settings, int direction);
const char *PcSettings_ControllerName(const PcSettings *settings);
const char *PcSettings_ButtonName(unsigned button);
void PcSettings_InputEvent(const SDL_Event *event);
void PcSettings_CloseInput(void);
PcViewport PcSettings_Viewport(Uint32 width, Uint32 height);
#endif
