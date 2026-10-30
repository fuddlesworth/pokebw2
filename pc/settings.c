#include "settings.h"
#include "gfl/key.h"
#include "nitro/host_card.h"
#include "save/config.h"

static PcSettings current = { .vsync = 1, .frameLimit = 1, .volume = 10, .controller = 1, .deadzone = 2 };
static const Uint16 buttons[PC_SETTINGS_KEYS] = { PAD_KEY_UP,   PAD_KEY_DOWN, PAD_KEY_LEFT,     PAD_KEY_RIGHT,
                                                  PAD_BUTTON_A, PAD_BUTTON_B, PAD_BUTTON_X,     PAD_BUTTON_Y,
                                                  PAD_BUTTON_L, PAD_BUTTON_R, PAD_BUTTON_START, PAD_BUTTON_SELECT };
static const Uint16 defaultKeys[PC_SETTINGS_KEYS] = { SDL_SCANCODE_UP,    SDL_SCANCODE_DOWN,   SDL_SCANCODE_LEFT,
                                                      SDL_SCANCODE_RIGHT, SDL_SCANCODE_Z,      SDL_SCANCODE_X,
                                                      SDL_SCANCODE_S,     SDL_SCANCODE_A,      SDL_SCANCODE_Q,
                                                      SDL_SCANCODE_W,     SDL_SCANCODE_RETURN, SDL_SCANCODE_BACKSPACE };
static const Uint16 defaultButtons[PC_SETTINGS_KEYS] = {
    SDL_GAMEPAD_BUTTON_DPAD_UP,        SDL_GAMEPAD_BUTTON_DPAD_DOWN, SDL_GAMEPAD_BUTTON_DPAD_LEFT,
    SDL_GAMEPAD_BUTTON_DPAD_RIGHT,     SDL_GAMEPAD_BUTTON_SOUTH,     SDL_GAMEPAD_BUTTON_EAST,
    SDL_GAMEPAD_BUTTON_NORTH,          SDL_GAMEPAD_BUTTON_WEST,      SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
    SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_START,     SDL_GAMEPAD_BUTTON_BACK
};
void PcSettings_Defaults(PcSettings *settings) {
    *settings =
        (PcSettings){ .vsync = 1, .frameLimit = 1, .volume = 10, .controller = 1, .deadzone = 2, .leftStick = 1 };
    SDL_memcpy(settings->keys, defaultKeys, sizeof(defaultKeys));
    SDL_memcpy(settings->controllerButtons, defaultButtons, sizeof(defaultButtons));
}
const PcSettings *PcSettings_Get(void) {
    return &current;
}
static bool Valid(const PcSettings *s) {
    if (s->leftStick > 1 || s->controllerGuid[32])
        return false;
    if (!s->controllerGuid[0] && (s->controllerSlot || s->controllerPathHash))
        return false;
    if (s->controllerGuid[0]) {
        if (SDL_strlen(s->controllerGuid) != 32)
            return false;
        for (unsigned i = 0; i < 32; i++)
            if (!SDL_isxdigit(s->controllerGuid[i]))
                return false;
    }
    for (unsigned i = 0; i < PC_SETTINGS_KEYS; i++) {
        if (s->controllerButtons[i] >= SDL_GAMEPAD_BUTTON_COUNT)
            return false;
        for (unsigned j = 0; j < i; j++)
            if (s->controllerButtons[i] == s->controllerButtons[j])
                return false;
    }
    if (s->fullscreen > 1 || s->resolution > 3 || s->vsync > 1 || s->frameLimit > 4 || s->volume > 10)
        return false;
    if (s->antialiasing > 2 || s->integerScale > 1 || s->controller > 1 || s->deadzone > 5 || s->invertWheel > 1 ||
        s->spriteFilter > 1)
        return false;
    if (s->gameValid > 1 || s->textSpeed > 2 || s->battleScene > 1 || s->battleStyle > 1 || s->saveBeforeIR > 1 ||
        s->sound > 1)
        return false;
    for (unsigned i = 0; i < PC_SETTINGS_KEYS; i++) {
        if (!s->keys[i] || s->keys[i] >= SDL_SCANCODE_COUNT || s->keys[i] == SDL_SCANCODE_ESCAPE)
            return false;
        for (unsigned j = 0; j < i; j++)
            if (s->keys[i] == s->keys[j])
                return false;
    }
    for (unsigned i = 0; i < PC_SETTINGS_KEYS; i++) {
        if (s->alternate[i] >= SDL_SCANCODE_COUNT || s->alternate[i] == SDL_SCANCODE_ESCAPE)
            return false;
        if (!s->alternate[i])
            continue;
        for (unsigned j = 0; j < PC_SETTINGS_KEYS; j++)
            if (s->alternate[i] == s->keys[j] || (i != j && s->alternate[i] == s->alternate[j]))
                return false;
    }
    return true;
}
void PcSettings_Load(void) {
    Uint8 data[PC_SETTINGS_RECORD_SIZE];
    PcSettings_Defaults(&current);
    if (!PcCard_ReadSettings(data, sizeof(data)) || data[0] < 1 || data[0] > 4)
        return;
    PcSettings loaded = {
        .fullscreen = data[1], .resolution = data[2], .vsync = data[3], .frameLimit = data[4], .volume = data[5]
    };
    for (unsigned i = 0; i < PC_SETTINGS_KEYS; i++)
        loaded.keys[i] = data[8 + 2 * i] | (Uint16)data[9 + 2 * i] << 8;
    loaded.gameValid = data[32];
    loaded.textSpeed = data[33];
    loaded.battleScene = data[34];
    loaded.battleStyle = data[35];
    loaded.saveBeforeIR = data[36];
    loaded.sound = data[37];
    loaded.controller = 1;
    loaded.deadzone = 2;
    loaded.leftStick = 1;
    SDL_memcpy(loaded.controllerButtons, defaultButtons, sizeof(defaultButtons));
    if (data[0] >= 2) {
        loaded.antialiasing = data[38];
        loaded.integerScale = data[39];
        loaded.controller = data[40];
        loaded.deadzone = data[41];
        loaded.invertWheel = data[42];
        loaded.spriteFilter = data[43];
        for (unsigned i = 0; i < PC_SETTINGS_KEYS; i++)
            loaded.alternate[i] = data[48 + 2 * i] | (Uint16)data[49 + 2 * i] << 8;
    }
    if (data[0] >= 3) {
        loaded.leftStick = data[72];
        for (unsigned i = 0; i < PC_SETTINGS_KEYS; i++)
            loaded.controllerButtons[i] = data[73 + i];
        SDL_memcpy(loaded.controllerGuid, data + 85, sizeof(loaded.controllerGuid));
    }
    if (data[0] >= 4) {
        loaded.controllerSlot = data[118];
        for (unsigned i = 0; i < 8; i++)
            loaded.controllerPathHash |= (Uint64)data[119 + i] << (i * 8);
    }
    if (Valid(&loaded))
        current = loaded;
}
bool PcSettings_Save(const PcSettings *settings) {
    if (!Valid(settings))
        return SDL_SetError("Invalid settings");
    Uint8 data[PC_SETTINGS_RECORD_SIZE] = {
        4, settings->fullscreen, settings->resolution, settings->vsync, settings->frameLimit, settings->volume
    };
    for (unsigned i = 0; i < PC_SETTINGS_KEYS; i++) {
        data[8 + 2 * i] = settings->keys[i] & 255;
        data[9 + 2 * i] = settings->keys[i] >> 8;
    }
    data[32] = settings->gameValid;
    data[33] = settings->textSpeed;
    data[34] = settings->battleScene;
    data[35] = settings->battleStyle;
    data[36] = settings->saveBeforeIR;
    data[37] = settings->sound;
    data[38] = settings->antialiasing;
    data[39] = settings->integerScale;
    data[40] = settings->controller;
    data[41] = settings->deadzone;
    data[42] = settings->invertWheel;
    data[43] = settings->spriteFilter;
    for (unsigned i = 0; i < PC_SETTINGS_KEYS; i++) {
        data[48 + 2 * i] = settings->alternate[i] & 255;
        data[49 + 2 * i] = settings->alternate[i] >> 8;
    }
    data[72] = settings->leftStick;
    for (unsigned i = 0; i < PC_SETTINGS_KEYS; i++)
        data[73 + i] = settings->controllerButtons[i];
    SDL_memcpy(data + 85, settings->controllerGuid, sizeof(settings->controllerGuid));
    data[118] = settings->controllerSlot;
    for (unsigned i = 0; i < 8; i++)
        data[119 + i] = settings->controllerPathHash >> (i * 8);
    if (!PcCard_WriteSettings(data, sizeof(data)))
        return false;
    current = *settings;
    return true;
}
bool PcSettings_ApplyVideo(PcVideo *video, const PcSettings *settings) {
    static const int sizes[][2] = { { 1280, 720 }, { 1600, 900 }, { 1920, 1080 }, { 2560, 1440 } };
    if (!Valid(settings))
        return SDL_SetError("Invalid display settings");
    SDL_GPUSampleCount samples = settings->antialiasing == 2   ? SDL_GPU_SAMPLECOUNT_4
                                 : settings->antialiasing == 1 ? SDL_GPU_SAMPLECOUNT_2
                                                               : SDL_GPU_SAMPLECOUNT_1;
    if (!SDL_GPUTextureSupportsSampleCount(video->device, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, samples) ||
        !SDL_GPUTextureSupportsSampleCount(video->device, SDL_GPU_TEXTUREFORMAT_D32_FLOAT, samples))
        return SDL_SetError("This GPU does not support the selected multisampling mode");
    SDL_GPUPresentMode mode = settings->vsync ? SDL_GPU_PRESENTMODE_VSYNC : SDL_GPU_PRESENTMODE_IMMEDIATE;
    if (!SDL_WindowSupportsGPUPresentMode(video->device, video->window, mode))
        return SDL_SetError("This display does not support the selected VSync mode");
    if (!SDL_SetGPUSwapchainParameters(video->device, video->window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, mode))
        return false;
    if (!SDL_SetWindowFullscreen(video->window, settings->fullscreen))
        return false;
    if (!settings->fullscreen &&
        !SDL_SetWindowSize(video->window, sizes[settings->resolution][0], sizes[settings->resolution][1]))
        return false;
    return true;
}
void PcSettings_LimitFrame(Uint64 start) {
    static const unsigned rates[] = { 0, 60, 120, 144, 240 };
    unsigned rate = rates[current.frameLimit];
    if (!rate)
        return;
    Uint64 target = start + SDL_NS_PER_SECOND / rate, now = SDL_GetTicksNS();
    if (target > now)
        SDL_DelayPrecise(target - now);
}
Uint16 PcSettings_Key(SDL_Scancode code) {
    for (unsigned i = 0; i < PC_SETTINGS_KEYS; i++)
        if (current.keys[i] == code || (code && current.alternate[i] == code))
            return buttons[i];
    return 0;
}
Uint16 PcSettings_Keyboard(void) {
    const bool *keys = SDL_GetKeyboardState(NULL);
    Uint16 pad = keys[SDL_SCANCODE_ESCAPE] ? PAD_BUTTON_B : 0;
    for (unsigned i = 0; i < PC_SETTINGS_KEYS; i++)
        if ((current.keys[i] && keys[current.keys[i]]) || (current.alternate[i] && keys[current.alternate[i]]))
            pad |= buttons[i];
    return pad;
}

void PcSettings_ApplyGame(Config *config) {
    if (!current.gameValid)
        return;
    Config_SetTextSpeed(config, current.textSpeed);
    Config_SetBattleScene(config, current.battleScene);
    Config_SetBattleStyle(config, current.battleStyle);
    Config_SetSaveBeforeIR(config, current.saveBeforeIR);
    Config_SetSoundFlags(config, current.sound);
}

PcViewport PcSettings_Viewport(Uint32 width, Uint32 height) {
    PcViewport viewport = PcViewport_Fit(width, height);
    if (current.integerScale) {
        unsigned scale = SDL_min(width / PC_CANVAS_WIDTH, height / PC_CANVAS_HEIGHT);
        if (scale) {
            viewport.width = PC_CANVAS_WIDTH * scale;
            viewport.height = PC_CANVAS_HEIGHT * scale;
            viewport.x = (width - viewport.width) / 2;
            viewport.y = (height - viewport.height) / 2;
        }
    }
    return viewport;
}
static SDL_Gamepad *gamepad;
static bool GuidMatches(SDL_JoystickID id, const PcSettings *settings) {
    char guid[33];
    SDL_GUIDToString(SDL_GetGamepadGUIDForID(id), guid, sizeof(guid));
    return !settings->controllerGuid[0] || !SDL_strcmp(guid, settings->controllerGuid);
}
static Uint64 ControllerPathHash(SDL_JoystickID id) {
    const unsigned char *path = (const unsigned char *)SDL_GetGamepadPathForID(id);
    if (!path || !*path)
        return 0;
    Uint64 hash = UINT64_C(14695981039346656037);
    for (; *path; path++)
        hash = (hash ^ *path) * UINT64_C(1099511628211);
    return hash;
}
static SDL_JoystickID PreferredController(const PcSettings *settings, const SDL_JoystickID *ids, int count) {
    unsigned slot = 0;
    SDL_JoystickID matched = 0, fallback = 0;
    for (int i = 0; i < count; i++) {
        if (!GuidMatches(ids[i], settings))
            continue;
        slot++;
        if (!fallback && (!settings->controllerSlot || slot == settings->controllerSlot))
            fallback = ids[i];
        if (settings->controllerPathHash) {
            if (ControllerPathHash(ids[i]) == settings->controllerPathHash) {
                matched = ids[i];
                if (slot == settings->controllerSlot)
                    return matched;
            }
        } else if (ids[i] == fallback)
            return ids[i];
    }
    return matched ? matched : fallback;
}
void PcSettings_InputEvent(const SDL_Event *event) {
    if (event->type == SDL_EVENT_GAMEPAD_ADDED && !gamepad)
        PcSettings_Controller();
    if (event->type == SDL_EVENT_GAMEPAD_REMOVED && gamepad && SDL_GetGamepadID(gamepad) == event->gdevice.which) {
        SDL_CloseGamepad(gamepad);
        gamepad = NULL;
    }
}
void PcSettings_CloseInput(void) {
    SDL_CloseGamepad(gamepad);
    gamepad = NULL;
}
SDL_Gamepad *PcSettings_Controller(void) {
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    SDL_JoystickID preferred = PreferredController(&current, ids, count);
    SDL_free(ids);
    if (gamepad && (!SDL_GamepadConnected(gamepad) || SDL_GetGamepadID(gamepad) != preferred))
        PcSettings_CloseInput();
    if (!gamepad && preferred)
        gamepad = SDL_OpenGamepad(preferred);
    return gamepad;
}
void PcSettings_CycleController(PcSettings *settings, int direction) {
    int count = 0, selected = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    SDL_JoystickID preferred = PreferredController(settings, ids, count);
    for (int i = 0; i < count; i++)
        if (settings->controllerGuid[0] && ids[i] == preferred) {
            selected = i + 1;
            break;
        }
    selected = (selected + count + 1 + direction) % (count + 1);
    SDL_zero(settings->controllerGuid);
    settings->controllerSlot = 0;
    settings->controllerPathHash = 0;
    if (selected) {
        SDL_GUIDToString(SDL_GetGamepadGUIDForID(ids[selected - 1]), settings->controllerGuid,
                         sizeof(settings->controllerGuid));
        for (int i = 0; i < selected; i++)
            if (GuidMatches(ids[i], settings))
                settings->controllerSlot++;
        settings->controllerPathHash = ControllerPathHash(ids[selected - 1]);
        if (settings->controllerPathHash)
            for (int i = 0; i < count; i++)
                if (i != selected - 1 && GuidMatches(ids[i], settings) &&
                    ControllerPathHash(ids[i]) == settings->controllerPathHash) {
                    settings->controllerPathHash = 0;
                    break;
                }
    }
    SDL_free(ids);
}
const char *PcSettings_ControllerName(const PcSettings *settings) {
    if (!settings->controllerGuid[0])
        return "Automatic";
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    SDL_JoystickID preferred = PreferredController(settings, ids, count);
    const char *name = "Disconnected";
    unsigned matching = 0, selectedSlot = 0;
    for (int i = 0; i < count; i++) {
        if (GuidMatches(ids[i], settings))
            matching++;
        if (ids[i] == preferred) {
            name = SDL_GetGamepadNameForID(ids[i]);
            selectedSlot = matching;
        }
    }
    SDL_free(ids);
    if (preferred && matching > 1) {
        static char named[128];
        SDL_snprintf(named, sizeof(named), "%s #%u", name ? name : "Unknown controller", selectedSlot);
        return named;
    }
    return name ? name : "Unknown controller";
}
const char *PcSettings_ButtonName(unsigned button) {
    switch (button) {
    case SDL_GAMEPAD_BUTTON_SOUTH:
        return "A / South";
    case SDL_GAMEPAD_BUTTON_EAST:
        return "B / East";
    case SDL_GAMEPAD_BUTTON_WEST:
        return "X / West";
    case SDL_GAMEPAD_BUTTON_NORTH:
        return "Y / North";
    case SDL_GAMEPAD_BUTTON_BACK:
        return "Back / View";
    case SDL_GAMEPAD_BUTTON_START:
        return "Start / Menu";
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
        return "LB / L1";
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
        return "RB / R1";
    case SDL_GAMEPAD_BUTTON_DPAD_UP:
        return "D-pad Up";
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        return "D-pad Down";
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
        return "D-pad Left";
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
        return "D-pad Right";
    default: {
        const char *name = SDL_GetGamepadStringForButton(button);
        return name ? name : "Unknown";
    }
    }
}
static Uint16 ControllerPad(bool menu) {
    if (!current.controller || !PcSettings_Controller())
        return 0;
    const Uint16 *mapping = menu ? defaultButtons : current.controllerButtons;
    Uint16 pad = 0;
    for (unsigned i = 0; i < PC_SETTINGS_KEYS; i++)
        if (SDL_GetGamepadButton(gamepad, mapping[i]))
            pad |= buttons[i];
    if (!menu && !current.leftStick)
        return pad;
    int deadzone = (current.deadzone + 1) * 32767 / 10;
    int x = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX),
        y = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);
    if (x < -deadzone)
        pad |= PAD_KEY_LEFT;
    if (x > deadzone)
        pad |= PAD_KEY_RIGHT;
    if (y < -deadzone)
        pad |= PAD_KEY_UP;
    if (y > deadzone)
        pad |= PAD_KEY_DOWN;
    return pad;
}
Uint16 PcSettings_Gamepad(void) {
    return ControllerPad(false);
}
Uint16 PcSettings_MenuGamepad(void) {
    return ControllerPad(true);
}
