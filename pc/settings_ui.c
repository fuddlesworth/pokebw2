#include "settings_ui.h"
#include "app/config_panel.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "save/config.h"
#include "settings.h"
#include "ui_components.h"
#include "ui_frame.h"

enum {
    CATEGORY_GAME,
    CATEGORY_GRAPHICS,
    CATEGORY_AUDIO,
    CATEGORY_KEYBOARD,
    CATEGORY_CONTROLLER,
    CATEGORY_ACCESSIBILITY,
    CATEGORY_COUNT
};
static const char *const categories[] = {
    "Game", "Graphics", "Audio", "Keyboard & Mouse", "Controller", "Accessibility"
};
static const char *const gameLabels[] = { "Text speed", "Battle animations", "Battle style", "Save before IR" };
static const char *const graphicsLabels[] = { "Display mode",        "Window resolution", "VSync",
                                              "Drawing frame limit", "3D anti-aliasing",  "Integer scaling",
                                              "2D sprite filtering" };
static const char *const keyLabels[] = { "Move up",       "Move down",      "Move left", "Move right",
                                         "Confirm",       "Back / Run",     "Open menu", "Registered item",
                                         "Left shoulder", "Right shoulder", "Start",     "Select" };
static struct {
    PcUiInput input;
    PcSettings draft;
    Uint8 game[4], savedGame[4], sound, savedSound;
    unsigned category, row, page, column;
    bool mousePage, controllerBindings;
    int conflict;
    Uint16 candidate;
    int binding;
    bool sidebar, initialized, confirmClose;
    PcVideo *video;
    char message[256];
} ui;

void PcSettingsUi_Reset(void) {
    SDL_zero(ui);
    ui.binding = -1;
    ui.conflict = -1;
}
static unsigned Phase(void) {
    return ui.category | (ui.page << 3) | ((unsigned)ui.mousePage << 4) | ((unsigned)(ui.conflict >= 0) << 5) |
           ((unsigned)ui.controllerBindings << 6);
}
static bool State(ConfigPanelNativeState *state) {
    if (!ConfigPanel_GetNativeState(state))
        return false;
    if (!ui.initialized) {
        PcSettingsUi_Reset();
        ui.initialized = true;
        ui.draft = *PcSettings_Get();
        ui.game[0] = Config_GetTextSpeed(state->config);
        ui.game[1] = Config_GetBattleScene(state->config);
        ui.game[2] = Config_GetBattleStyle(state->config);
        ui.game[3] = Config_GetSaveBeforeIR(state->config);
        SDL_memcpy(ui.savedGame, ui.game, sizeof(ui.game));
        ui.sound = ui.savedSound = getSoundFlags(state->config);
        ui.sidebar = true;
    }
    PcUiInput_Sync(&ui.input, state->font, Phase());
    return true;
}
static bool Dirty(void) {
    const PcSettings *saved = PcSettings_Get();
    return ui.draft.fullscreen != saved->fullscreen || ui.draft.resolution != saved->resolution ||
           ui.draft.vsync != saved->vsync || ui.draft.frameLimit != saved->frameLimit ||
           ui.draft.volume != saved->volume || ui.draft.antialiasing != saved->antialiasing ||
           ui.draft.integerScale != saved->integerScale || ui.draft.spriteFilter != saved->spriteFilter ||
           ui.draft.controller != saved->controller || ui.draft.deadzone != saved->deadzone ||
           ui.draft.leftStick != saved->leftStick || ui.draft.controllerSlot != saved->controllerSlot ||
           ui.draft.controllerPathHash != saved->controllerPathHash ||
           SDL_strcmp(ui.draft.controllerGuid, saved->controllerGuid) ||
           SDL_memcmp(ui.draft.controllerButtons, saved->controllerButtons, sizeof(ui.draft.controllerButtons)) ||
           ui.draft.invertWheel != saved->invertWheel ||
           SDL_memcmp(ui.draft.alternate, saved->alternate, sizeof(ui.draft.alternate)) ||
           SDL_memcmp(ui.draft.keys, saved->keys, sizeof(ui.draft.keys)) ||
           SDL_memcmp(ui.game, ui.savedGame, sizeof(ui.game)) || ui.sound != ui.savedSound;
}
static unsigned RowCount(void) {
    static const unsigned counts[] = { 4, 7, 2, 6, 4, 1 };
    if (ui.category == CATEGORY_CONTROLLER && ui.controllerBindings)
        return 6;
    return ui.category == CATEGORY_KEYBOARD && ui.mousePage ? 1 : counts[ui.category];
}
static const char *Label(unsigned row) {
    switch (ui.category) {
    case CATEGORY_GAME:
        return gameLabels[row];
    case CATEGORY_GRAPHICS:
        return graphicsLabels[row];
    case CATEGORY_AUDIO:
        return row ? "Sound output" : "Master volume";
    case CATEGORY_KEYBOARD:
        return ui.mousePage ? "Mouse wheel direction" : keyLabels[row + ui.page * 6];
    case CATEGORY_CONTROLLER:
        if (ui.controllerBindings)
            return keyLabels[row + ui.page * 6];
        return (const char *const[]){ "Controller input", "Preferred device", "Left stick movement",
                                      "Stick deadzone" }[row];
    case CATEGORY_ACCESSIBILITY:
        return "Text speed";
    default:
        return "Controller";
    }
}
static void Value(unsigned row, char *value, size_t size) {
    static const char *const speed[] = { "Slow", "Normal", "Fast" };
    static const char *const resolutions[] = { "1280 x 720", "1600 x 900", "1920 x 1080", "2560 x 1440" };
    static const char *const rates[] = { "Unlimited", "60 FPS", "120 FPS", "144 FPS", "240 FPS" };
    const char *text = "";
    switch (ui.category) {
    case CATEGORY_GAME:
        if (row == 0)
            text = speed[SDL_min(ui.game[0], 2)];
        else if (row == 2)
            text = ui.game[2] ? "Set" : "Shift";
        else
            text = ui.game[row] ? (row == 1 ? "Off" : "On") : (row == 1 ? "On" : "Off");
        break;
    case CATEGORY_GRAPHICS:
        if (row == 0)
            text = ui.draft.fullscreen ? "Fullscreen" : "Windowed";
        if (row == 1)
            text = resolutions[ui.draft.resolution];
        if (row == 2)
            text = ui.draft.vsync ? "On" : "Off";
        if (row == 3)
            text = rates[ui.draft.frameLimit];
        if (row == 4) {
            const char *aa[] = { "Off", "2x MSAA", "4x MSAA" };
            text = aa[ui.draft.antialiasing];
        }
        if (row == 5)
            text = ui.draft.integerScale ? "On" : "Off";
        if (row == 6)
            text = ui.draft.spriteFilter ? "Linear" : "Nearest";
        break;
    case CATEGORY_AUDIO:
        if (!row) {
            SDL_snprintf(value, size, "%u%%", ui.draft.volume * 10);
            return;
        }
        text = ui.sound ? "Mono" : "Stereo";
        break;
    case CATEGORY_KEYBOARD:
        text = ui.draft.invertWheel ? "Reversed" : "Normal";
        break;
    case CATEGORY_CONTROLLER:
        if (!row)
            text = ui.draft.controller ? "On" : "Off";
        else if (row == 1)
            text = PcSettings_ControllerName(&ui.draft);
        else if (row == 2)
            text = ui.draft.leftStick ? "On" : "Off";
        else {
            SDL_snprintf(value, size, "%u%%", (ui.draft.deadzone + 1) * 10);
            return;
        }
        break;
    case CATEGORY_ACCESSIBILITY:
        text = speed[SDL_min(ui.game[0], 2)];
        break;
    default:
        break;
    }
    SDL_strlcpy(value, text, size);
}
static void Change(int delta) {
    unsigned row = ui.row;
    Uint8 *value = NULL;
    unsigned count = 2;
    switch (ui.category) {
    case CATEGORY_GAME:
        value = &ui.game[row];
        count = row ? 2 : 3;
        break;
    case CATEGORY_GRAPHICS:
        if (row == 0)
            value = &ui.draft.fullscreen;
        if (row == 1) {
            value = &ui.draft.resolution;
            count = 4;
        }
        if (row == 2)
            value = &ui.draft.vsync;
        if (row == 3) {
            value = &ui.draft.frameLimit;
            count = 5;
        }
        if (row == 4) {
            value = &ui.draft.antialiasing;
            count = 3;
        }
        if (row == 5)
            value = &ui.draft.integerScale;
        if (row == 6)
            value = &ui.draft.spriteFilter;
        break;
    case CATEGORY_CONTROLLER:
        if (ui.controllerBindings) {
            ui.binding = row + ui.page * 6;
            ui.column = 0;
        } else if (row == 1)
            PcSettings_CycleController(&ui.draft, delta);
        else {
            value = row == 0 ? &ui.draft.controller : row == 2 ? &ui.draft.leftStick : &ui.draft.deadzone;
            count = row == 3 ? 6 : 2;
        }
        break;
    case CATEGORY_AUDIO:
        value = row ? &ui.sound : &ui.draft.volume;
        count = row ? 2 : 11;
        break;
    case CATEGORY_KEYBOARD:
        if (ui.mousePage)
            value = &ui.draft.invertWheel;
        else
            ui.binding = row + ui.page * 6 + ui.column * PC_SETTINGS_KEYS;
        break;
    case CATEGORY_ACCESSIBILITY:
        value = &ui.game[0];
        count = 3;
        break;
    default:
        break;
    }
    if (value)
        *value = (*value + count + delta) % count;
    ui.message[0] = 0;
    ui.confirmClose = false;
}
static void Apply(ConfigPanelNativeState *state) {
    if (!ui.video) {
        SDL_strlcpy(ui.message, "Use Enter or click Apply.", sizeof(ui.message));
        return;
    }
    ui.draft.gameValid = 1;
    ui.draft.textSpeed = ui.game[0];
    ui.draft.battleScene = ui.game[1];
    ui.draft.battleStyle = ui.game[2];
    ui.draft.saveBeforeIR = ui.game[3];
    ui.draft.sound = ui.sound;
    const PcSettings *saved = PcSettings_Get();
    bool displayChanged = ui.draft.fullscreen != saved->fullscreen || ui.draft.resolution != saved->resolution ||
                          ui.draft.vsync != saved->vsync || ui.draft.antialiasing != saved->antialiasing;
    if ((displayChanged && !PcSettings_ApplyVideo(ui.video, &ui.draft)) || !PcSettings_Save(&ui.draft)) {
        SDL_strlcpy(ui.message, SDL_GetError(), sizeof(ui.message));
        if (displayChanged)
            PcSettings_ApplyVideo(ui.video, PcSettings_Get());
        return;
    }
    Config_SetTextSpeed(state->config, ui.game[0]);
    Config_SetBattleScene(state->config, ui.game[1]);
    Config_SetBattleStyle(state->config, ui.game[2]);
    Config_SetSaveBeforeIR(state->config, ui.game[3]);
    Config_SetSoundFlags(state->config, ui.sound);
    GFL_SndSetStereo(ui.sound == 0);
    SDL_memcpy(ui.savedGame, ui.game, sizeof(ui.game));
    ui.savedSound = ui.sound;
    ui.confirmClose = false;
    SDL_strlcpy(ui.message, "Settings saved.", sizeof(ui.message));
}
static Uint16 *Binding(unsigned slot) {
    if (ui.category == CATEGORY_CONTROLLER)
        return &ui.draft.controllerButtons[slot];
    return slot < PC_SETTINGS_KEYS ? &ui.draft.keys[slot] : &ui.draft.alternate[slot - PC_SETTINGS_KEYS];
}
static const char *BindingName(unsigned slot) {
    if (ui.category == CATEGORY_CONTROLLER)
        return PcSettings_ButtonName(*Binding(slot));
    return *Binding(slot) ? SDL_GetScancodeName(*Binding(slot)) : "Unbound";
}
static void FinishBinding(bool swap) {
    if (swap) {
        if (ui.conflict >= 0) {
            if (ui.category != CATEGORY_CONTROLLER && ui.conflict < PC_SETTINGS_KEYS && !*Binding(ui.binding)) {
                SDL_strlcpy(ui.message, "A primary binding cannot be cleared. Choose another key.", sizeof(ui.message));
                ui.conflict = -1;
                return;
            }
            *Binding(ui.conflict) = *Binding(ui.binding);
        }
        *Binding(ui.binding) = ui.candidate;
        ui.confirmClose = false;
    }
    ui.binding = ui.conflict = -1;
    ui.input.previousKeys = 0xffff;
    ui.input.pendingKeys = 0;
}
static void Action(PcUiAction action, ConfigPanelNativeState *state) {
    if (ui.conflict >= 0) {
        if (action.kind == PC_UI_ACTION_CUSTOM && (action.value == 6 || action.value == 7))
            FinishBinding(action.value == 6);
        return;
    }
    if (ui.binding >= 0) {
        if (action.kind == PC_UI_ACTION_CUSTOM && action.value == 7)
            FinishBinding(false);
        return;
    }
    if (action.kind == PC_UI_ACTION_MARK) {
        ui.column = action.value / PC_SETTINGS_KEYS;
        ui.row = action.value % PC_SETTINGS_KEYS - ui.page * 6;
        ui.sidebar = false;
        ui.binding = action.value;
    } else if (action.kind == PC_UI_ACTION_TAB) {
        ui.category = action.value;
        ui.row = 0;
        ui.sidebar = false;
        ui.confirmClose = false;
    } else if (action.kind == PC_UI_ACTION_ROW) {
        ui.row = action.value;
        ui.sidebar = false;
        Change(1);
    } else if (action.kind == PC_UI_ACTION_CUSTOM) {
        if (action.value == 0)
            Apply(state);
        if (action.value == 1) {
            ui.draft = *PcSettings_Get();
            SDL_memcpy(ui.game, ui.savedGame, sizeof(ui.game));
            ui.sound = ui.savedSound;
            ui.message[0] = 0;
            ui.confirmClose = false;
        }
        if (action.value == 2) {
            PcSettings_Defaults(&ui.draft);
            ui.game[0] = 1;
            ui.game[1] = 0;
            ui.game[2] = 0;
            ui.game[3] = 1;
            ui.sound = 0;
            ui.message[0] = 0;
            ui.confirmClose = false;
        }
        if (action.value == 4 || action.value == 5) {
            ui.mousePage = action.value == 5;
            ui.row = 0;
        }
        if (action.value == 8 || action.value == 9) {
            ui.controllerBindings = action.value == 9;
            ui.row = ui.page = ui.column = 0;
        }
        if (action.value == 3) {
            ui.page ^= 1;
            ui.row = 0;
        }
    }
}
Uint16 PcSettingsUi_FilterKeys(Uint16 keys) {
    ConfigPanelNativeState state;
    if (!State(&state))
        return keys;
    Uint16 pressed;
    PcUiAction action;
    PcUiInput_Read(&ui.input, keys, &pressed, &action);
    if (ui.conflict >= 0) {
        if (pressed & PAD_BUTTON_A)
            FinishBinding(true);
        else if (pressed & PAD_BUTTON_B)
            FinishBinding(false);
        else
            Action(action, &state);
        return 0;
    }
    if (ui.binding >= 0) {
        Action(action, &state);
        return 0;
    }
    Action(action, &state);
    if (pressed & PAD_BUTTON_B) {
        if (Dirty() && !ui.confirmClose) {
            ui.confirmClose = true;
            SDL_strlcpy(ui.message, "Unsaved changes. Press Back again to discard, or Apply.", sizeof(ui.message));
        } else
            ConfigPanel_CloseNative();
    }
    if (pressed & (PAD_BUTTON_L | PAD_BUTTON_R)) {
        ui.category = (ui.category + CATEGORY_COUNT + ((pressed & PAD_BUTTON_L) ? -1 : 1)) % CATEGORY_COUNT;
        ui.row = 0;
        ui.sidebar = false;
    }
    if (ui.category == CATEGORY_CONTROLLER && (pressed & PAD_BUTTON_X)) {
        ui.controllerBindings = !ui.controllerBindings;
        ui.row = ui.page = ui.column = 0;
    }
    if (ui.category == CATEGORY_CONTROLLER && ui.controllerBindings && (pressed & PAD_BUTTON_SELECT)) {
        ui.page ^= 1;
        ui.row = 0;
    }
    unsigned count = RowCount();
    if (pressed & (PAD_KEY_UP | PAD_KEY_DOWN)) {
        int step = (pressed & PAD_KEY_UP) ? -1 : 1;
        if (ui.sidebar) {
            ui.category = (ui.category + CATEGORY_COUNT + step) % CATEGORY_COUNT;
            ui.row = 0;
        } else
            ui.row = (ui.row + count + 3 + step) % (count + 3);
    }
    if (pressed & (PAD_KEY_LEFT | PAD_KEY_RIGHT)) {
        if (ui.sidebar) {
            ui.sidebar = false;
            ui.row = 0;
        } else if (ui.category == CATEGORY_KEYBOARD && !ui.mousePage && ui.row < count)
            ui.column = (pressed & PAD_KEY_RIGHT) ? 1 : 0;
        else if (ui.row < count)
            Change((pressed & PAD_KEY_LEFT) ? -1 : 1);
    }
    if (pressed & PAD_BUTTON_A) {
        if (ui.sidebar)
            ui.sidebar = false;
        else if (ui.row < count)
            Change(1);
        else
            Action((PcUiAction){ PC_UI_ACTION_CUSTOM, (int)(ui.row - count) }, &state);
    }
    return 0;
}
bool PcSettingsUi_HandleEvent(PcVideo *video, const SDL_Event *event) {
    ConfigPanelNativeState state;
    if (!State(&state))
        return false;
    ui.video = video;
    if (ui.category == CATEGORY_CONTROLLER && ui.binding >= 0 && ui.conflict < 0 &&
        event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        SDL_Gamepad *controller = PcSettings_Controller();
        if (!controller || event->gbutton.which != SDL_GetGamepadID(controller))
            return true;
        ui.candidate = event->gbutton.button;
        for (unsigned i = 0; i < PC_SETTINGS_KEYS; i++)
            if ((int)i != ui.binding && *Binding(i) == ui.candidate) {
                ui.conflict = i;
                break;
            }
        ui.input.previousKeys = 0xffff;
        ui.input.pendingKeys = 0;
        if (ui.conflict < 0)
            FinishBinding(true);
        return true;
    }
    if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
        if (ui.conflict >= 0) {
            if (event->key.scancode == SDL_SCANCODE_RETURN)
                FinishBinding(true);
            if (event->key.scancode == SDL_SCANCODE_ESCAPE)
                FinishBinding(false);
            return true;
        }
        if (ui.binding >= 0) {
            SDL_Scancode key = event->key.scancode;
            if (key == SDL_SCANCODE_ESCAPE) {
                FinishBinding(false);
                return true;
            }
            if (ui.category == CATEGORY_CONTROLLER)
                return true;
            if (key > SDL_SCANCODE_UNKNOWN && key < SDL_SCANCODE_COUNT) {
                ui.candidate = key;
                ui.conflict = -1;
                for (unsigned i = 0; i < PC_SETTINGS_KEYS * 2; i++)
                    if ((int)i != ui.binding && *Binding(i) == key) {
                        ui.conflict = i;
                        break;
                    }
                if (ui.conflict < 0)
                    FinishBinding(true);
            }
            return true;
        }
        if (event->key.scancode == SDL_SCANCODE_DELETE && ui.category == CATEGORY_KEYBOARD && !ui.mousePage &&
            ui.row < 6 && ui.column) {
            ui.draft.alternate[ui.row + ui.page * 6] = 0;
            ui.confirmClose = false;
            return true;
        }
        if (event->key.scancode == SDL_SCANCODE_TAB) {
            ui.sidebar = !ui.sidebar;
            return true;
        }
        if (event->key.scancode == SDL_SCANCODE_PAGEDOWN || event->key.scancode == SDL_SCANCODE_PAGEUP) {
            if (ui.category == CATEGORY_KEYBOARD || (ui.category == CATEGORY_CONTROLLER && ui.controllerBindings))
                ui.page ^= 1;
            return true;
        }
    }
    return PcUiInput_Event(&ui.input, video, event);
}
static void Help(Font *font) {
    const char *lines[5] = { "", "", "", "", "" };
    if (ui.category == CATEGORY_GRAPHICS) {
        const char *help[][5] = { { "Use the entire display or", "play in a resizable window.",
                                    "Fullscreen uses the desktop", "resolution." },
                                  { "Choose the window size.", "The widescreen interface", "scales with the window.",
                                    "Fullscreen uses desktop size." },
                                  { "Synchronize drawing with", "the display to avoid tearing.",
                                    "Disable to allow higher", "drawing frame rates." },
                                  { "Limit drawing frames per", "second. Game logic, audio",
                                    "and animation retain their", "original fixed timing." } };
        for (unsigned i = 0; i < 5; i++)
            lines[i] = help[SDL_min(ui.row, 3)][i];
        if (ui.row == 4) {
            lines[0] = "Smooth polygon edges while";
            lines[1] = "keeping native menus crisp.";
            lines[2] = "Choose 2x or 4x multisampling.";
            lines[3] = "";
        }
        if (ui.row == 5) {
            lines[0] = "Use whole-number scaling of";
            lines[1] = "the 384 x 216 game canvas.";
            lines[2] = "Unused window space is black.";
            lines[3] = "Small windows use fit scaling.";
        }
        if (ui.row == 6) {
            lines[0] = "Choose nearest or linear";
            lines[1] = "filtering for hardware 2D";
            lines[2] = "sprites. Native menus and";
            lines[3] = "text remain crisp.";
        }
    } else if (ui.category == CATEGORY_KEYBOARD) {
        lines[0] = "Select a binding, then press";
        lines[1] = "a key. Escape cancels.";
        lines[2] = "Conflicts ask before swapping.";
        lines[3] = "Menu shortcuts stay unchanged.";
        lines[4] = "Page Up / Down: more bindings.";
        if (ui.mousePage) {
            lines[0] = "Choose normal or reversed";
            lines[1] = "wheel navigation for menus.";
            lines[2] = "Pointer movement and clicks";
            lines[3] = "follow the displayed canvas.";
            lines[4] = "";
        }
    } else if (ui.category == CATEGORY_CONTROLLER) {
        lines[0] = ui.controllerBindings ? "Select an action, then press" : "Choose a device, then Apply.";
        lines[1] = ui.controllerBindings ? "a button. Escape cancels." : "The preview shows live input.";
        lines[2] = "Menus: A confirm, B back.";
        lines[3] = "Y: Device / Bindings tab.";
        lines[4] = ui.controllerBindings ? "View: next bindings page." : "LB / RB: change category.";
    } else if (ui.category == CATEGORY_AUDIO) {
        lines[0] = "Adjust overall game volume";
        lines[1] = "and stereo / mono output.";
        lines[2] = "Apply to hear your changes.";
    } else {
        lines[0] = "Adjust the original game's";
        lines[1] = "options. Apply changes here,";
        lines[2] = "to save these preferences";
        lines[3] = "alongside your game data.";
    }
    for (unsigned i = 0; i < 5; i++)
        if (lines[i])
            PcUi_LineLabel(font, lines[i], 890, 465 + i * 24, 1235, PC_UI_INK, PC_UI_TEXT_LABEL);
}
bool PcSettingsUi_Draw(void) {
    ConfigPanelNativeState state;
    if (!State(&state))
        return false;
    Font *font = state.font;
    PcUi_BeginControls(font, Phase());
    PcMenuSkin_Add(PC_MENU_HEADER, 0, 0, 1280, 81, 0);
    PcMenuSkin_Add(PC_MENU_SETTINGS, 0, 81, 1280, 565, 0);
    PcMenuSkin_Add(PC_MENU_FOOTER, 0, 646, 1280, 74, 0);
    PcUiFrame_Header(font, "SETTINGS", "OPTIONS");
    for (unsigned i = 0; i < CATEGORY_COUNT; i++) {
        PcUiRect rect = { 24, 112 + i * 70, 248, 60 };
        if (i == ui.category)
            PcMenuSkin_Add(PC_MENU_SETTINGS_SELECTION, rect.x, rect.y, rect.width, rect.height, 0);
        PcUi_Control(PcUi_SkinShape(PC_MENU_ROW, rect), (PcUiAction){ PC_UI_ACTION_TAB, i });
        PcUi_LineLabel(font, categories[i], 86, rect.y + 18, 260, i == ui.category ? PC_UI_CYAN : PC_UI_WHITE,
                       PC_UI_TEXT_BUTTON);
    }
    PcUi_LineLabel(font, categories[ui.category], 320, 111, 820, PC_UI_INK, PC_UI_TEXT_PANEL_TITLE);
    if (ui.category != CATEGORY_KEYBOARD && ui.category != CATEGORY_CONTROLLER)
        PcUi_LineLabel(font, ui.category == CATEGORY_GRAPHICS ? "DISPLAY" : "PREFERENCES", 325, 176, 800, PC_UI_WHITE,
                       PC_UI_TEXT_BUTTON);
    bool controller = ui.category == CATEGORY_CONTROLLER;
    bool bindings = (ui.category == CATEGORY_KEYBOARD && !ui.mousePage) || (controller && ui.controllerBindings);
    if (controller) {
        PcUi_CenteredButtonLabel(font, "Device", (PcUiRect){ 312, 160, 248, 38 }, !ui.controllerBindings, true,
                                 (PcUiAction){ PC_UI_ACTION_CUSTOM, 8 });
        PcUi_CenteredButtonLabel(font, "Bindings", (PcUiRect){ 574, 160, 248, 38 }, ui.controllerBindings, true,
                                 (PcUiAction){ PC_UI_ACTION_CUSTOM, 9 });
        if (bindings) {
            PcUi_LineLabel(font, "Action", 328, 210, 520, PC_UI_INK, PC_UI_TEXT_LABEL);
            PcUi_LineCenteredLabel(font, "Button", 542, 210, 805, PC_UI_INK, PC_UI_TEXT_LABEL);
        }
    }
    if (ui.category == CATEGORY_KEYBOARD) {
        PcUi_CenteredButtonLabel(font, "Bindings", (PcUiRect){ 312, 160, 248, 38 }, !ui.mousePage, true,
                                 (PcUiAction){ PC_UI_ACTION_CUSTOM, 4 });
        PcUi_CenteredButtonLabel(font, "Mouse", (PcUiRect){ 574, 160, 248, 38 }, ui.mousePage, true,
                                 (PcUiAction){ PC_UI_ACTION_CUSTOM, 5 });
        if (bindings) {
            PcUi_LineLabel(font, "Action", 328, 210, 520, PC_UI_INK, PC_UI_TEXT_LABEL);
            PcUi_LineCenteredLabel(font, "Primary", 542, 210, 665, PC_UI_INK, PC_UI_TEXT_LABEL);
            PcUi_LineCenteredLabel(font, "Alternate", 682, 210, 805, PC_UI_INK, PC_UI_TEXT_LABEL);
        }
    }
    for (unsigned i = 0; i < RowCount(); i++) {
        if (bindings) {
            PcUiRect rect = { 312, 245 + i * 51, 510, 46 };
            PcMenuSkin_Add(PC_MENU_LIGHT_ROW, rect.x, rect.y, rect.width, rect.height, !ui.sidebar && ui.row == i);
            PcUi_LineLabel(font, Label(i), 328, rect.y + 12, 528, PC_UI_INK, PC_UI_TEXT_LABEL);
            for (unsigned column = 0; column < (controller ? 1 : 2); column++) {
                unsigned slot = i + ui.page * 6 + column * PC_SETTINGS_KEYS;
                const char *label =
                    ui.binding == (int)slot ? (controller ? "Press button..." : "Press key...") : BindingName(slot);
                PcUi_CenteredButtonLabel(
                    font, label, (PcUiRect){ 542 + column * 140, rect.y + 5, controller ? 272 : 132, 36 },
                    !ui.sidebar && ui.row == i && ui.column == column, true, (PcUiAction){ PC_UI_ACTION_MARK, slot });
            }
            continue;
        }
        bool graphics = ui.category == CATEGORY_GRAPHICS;
        if (graphics && i == 4) {
            PcMenuSkin_Add(PC_MENU_BUTTON, 312, 410, 510, 32, 0);
            PcUi_LineLabel(font, "IMAGE QUALITY", 328, 415, 800, PC_UI_WHITE, PC_UI_TEXT_LABEL);
        }
        PcUiRect rect = { 312, graphics ? 211 + i * 48 + (i >= 4 ? 46 : 0) : 211 + i * 59, 510, graphics ? 44 : 50 };
        PcUi_SkinControl(PC_MENU_LIGHT_ROW, rect, !ui.sidebar && ui.row == i, (PcUiAction){ PC_UI_ACTION_ROW, i });
        PcUi_LineLabel(font, Label(i), rect.x + 16, rect.y + 14, 592, PC_UI_INK, PC_UI_TEXT_LABEL);
        PcMenuSkin_Add(PC_MENU_BUTTON, 610, rect.y + 7, 204, 36, 0);
        char value[64];
        Value(i, value, sizeof(value));
        PcUi_LineCenteredLabel(font, value, 618, rect.y + 15, 800, PC_UI_WHITE, PC_UI_TEXT_LABEL);
    }
    if (bindings)
        PcUi_CenteredButtonLabel(font, ui.page ? "Previous bindings" : "More bindings", (PcUiRect){ 550, 576, 272, 40 },
                                 false, true, (PcUiAction){ PC_UI_ACTION_CUSTOM, 3 });
    if (bindings) {
        unsigned slot = SDL_min(ui.row, 5) + ui.page * 6 + ui.column * PC_SETTINGS_KEYS;
        if (controller)
            slot = SDL_min(ui.row, 5) + ui.page * 6;
        const char *label = ui.conflict >= 0
                                ? (controller ? PcSettings_ButtonName(ui.candidate) : SDL_GetScancodeName(ui.candidate))
                                : BindingName(slot);
        PcMenuSkin_Add(PC_MENU_BUTTON, 938, 190, 248, 100, ui.binding >= 0);
        PcUi_LineCenteredLabel(font, label, 950, 220, 1170, ui.binding >= 0 ? PC_UI_INK : PC_UI_WHITE,
                               PC_UI_TEXT_HERO_VALUE);
    }
    if (controller && !bindings) {
        SDL_Gamepad *pad = PcSettings_Controller();
        PcMenuSkin_Add(PC_MENU_BUTTON, 876, 163, 369, 140, 0);
        PcUi_LineCenteredLabel(font, pad ? SDL_GetGamepadName(pad) : "NO CONTROLLER CONNECTED", 890, 177, 1235,
                               PC_UI_WHITE, PC_UI_TEXT_LABEL);
        char axes[80];
        SDL_snprintf(axes, sizeof(axes), "Left stick  X %d%%  Y %d%%",
                     pad ? SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX) * 100 / 32768 : 0,
                     pad ? SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY) * 100 / 32768 : 0);
        PcUi_LineCenteredLabel(font, axes, 890, 218, 1235, PC_UI_WHITE, PC_UI_TEXT_LABEL);
        const char *held = "No buttons pressed";
        for (unsigned i = 0; pad && i < SDL_GAMEPAD_BUTTON_COUNT; i++)
            if (SDL_GetGamepadButton(pad, i)) {
                held = PcSettings_ButtonName(i);
                break;
            }
        PcUi_LineCenteredLabel(font, held, 890, 259, 1235, PC_UI_CYAN, PC_UI_TEXT_BUTTON);
    }
    PcUi_LineCenteredLabel(font,
                           ui.conflict >= 0  ? "CONFIRM SWAP"
                           : ui.binding >= 0 ? (controller ? "PRESS A BUTTON" : "PRESS A KEY")
                           : bindings        ? "SELECT A BINDING"
                           : controller      ? "LIVE INPUT"
                                             : "PERSONALIZE YOUR GAME",
                           890, 319, 1235, PC_UI_WHITE, PC_UI_TEXT_BUTTON);
    const char *heading = ui.conflict >= 0 ? "CONFLICT PREVIEW" : ui.row < RowCount() ? Label(ui.row) : "APPLY CHANGES";
    PcUi_LineLabel(font, heading, 890, 405, 1235, PC_UI_CYAN, PC_UI_TEXT_BUTTON);
    if (ui.conflict >= 0) {
        PcUi_LineLabel(font, controller ? "BUTTON ALREADY ASSIGNED" : "KEY ALREADY ASSIGNED", 890, 465, 1235, PC_UI_INK,
                       PC_UI_TEXT_BUTTON);
        PcUi_LineLabel(font, keyLabels[ui.conflict % PC_SETTINGS_KEYS], 890, 500, 1235, PC_UI_INK, PC_UI_TEXT_LABEL);
        PcUi_CenteredButtonLabel(font, "Swap", (PcUiRect){ 890, 545, 155, 42 }, true, true,
                                 (PcUiAction){ PC_UI_ACTION_CUSTOM, 6 });
        PcUi_CenteredButtonLabel(font, "Cancel", (PcUiRect){ 1065, 545, 170, 42 }, false, true,
                                 (PcUiAction){ PC_UI_ACTION_CUSTOM, 7 });
    } else {
        Help(font);
        if (ui.binding >= 0)
            PcUi_CenteredButtonLabel(font, "Cancel", (PcUiRect){ 1065, 585, 170, 32 }, false, true,
                                     (PcUiAction){ PC_UI_ACTION_CUSTOM, 7 });
    }
    PcUi_LineLabel(font,
                   ui.message[0] ? ui.message
                   : Dirty()     ? "Unsaved changes"
                                 : "All changes applied",
                   306, 621, 1238, PC_UI_INK, PC_UI_TEXT_CAPTION);
    const char *buttons[] = { "Apply", "Revert", "Restore defaults" };
    for (unsigned i = 0; i < 3; i++)
        PcUi_CenteredButtonLabel(font, buttons[i], (PcUiRect){ 284 + i * 182, 667, 170, 40 },
                                 !ui.sidebar && ui.row == RowCount() + i, true, (PcUiAction){ PC_UI_ACTION_CUSTOM, i });
    PcUiFrame_KeyHint(font, "Esc", "Back", 1250, 686, 44, PAD_BUTTON_B);
    PcUiFrame_KeyHint(font, "Enter", "Select", 1114, 686, 64, PAD_BUTTON_A);
    PcUi_LineLabel(font, "Tab: Categories", 28, 679, 264, PC_UI_MUTED, PC_UI_TEXT_LABEL);
    return true;
}
