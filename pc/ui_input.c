#include "ui_input.h"
#include "gfl/key.h"
#include "settings.h"
#include "ui_components.h"

static Uint16 MenuKey(SDL_Scancode code) {
    switch (code) {
    case SDL_SCANCODE_TAB:
        return PAD_BUTTON_R;
    case SDL_SCANCODE_T:
        return PAD_BUTTON_START;
    case SDL_SCANCODE_A:
        return PAD_BUTTON_SELECT;
    case SDL_SCANCODE_D:
        return PAD_BUTTON_Y;
    default:
        return 0;
    }
}

Uint16 PcUi_MenuEventKey(const SDL_Event *event) {
    if (event->type == SDL_EVENT_KEY_DOWN && !event->key.repeat) {
        Uint16 key = MenuKey(event->key.scancode);
        if (key)
            return key;
    }
    return PcUi_EventKey(event);
}

Uint16 PcUi_KeyboardPad(bool menu) {
    if (!menu)
        return PcSettings_Keyboard() | PcSettings_Gamepad();
    const bool *pressed = SDL_GetKeyboardState(NULL);
    static const struct {
        SDL_Scancode code;
        Uint16 button;
    } bindings[] = { { SDL_SCANCODE_Z, PAD_BUTTON_A },          { SDL_SCANCODE_X, PAD_BUTTON_B },
                     { SDL_SCANCODE_RETURN, PAD_BUTTON_START }, { SDL_SCANCODE_BACKSPACE, PAD_BUTTON_SELECT },
                     { SDL_SCANCODE_UP, PAD_KEY_UP },           { SDL_SCANCODE_DOWN, PAD_KEY_DOWN },
                     { SDL_SCANCODE_LEFT, PAD_KEY_LEFT },       { SDL_SCANCODE_RIGHT, PAD_KEY_RIGHT },
                     { SDL_SCANCODE_A, PAD_BUTTON_Y },          { SDL_SCANCODE_S, PAD_BUTTON_X },
                     { SDL_SCANCODE_Q, PAD_BUTTON_L },          { SDL_SCANCODE_W, PAD_BUTTON_R } };
    Uint16 keys = PcSettings_MenuGamepad();
    for (unsigned i = 0; i < SDL_arraysize(bindings); i++) {
        if (!pressed[bindings[i].code])
            continue;
        SDL_Event event = { .key = { .type = SDL_EVENT_KEY_DOWN } };
        event.key.scancode = bindings[i].code;
        keys |= menu ? PcUi_MenuEventKey(&event) : bindings[i].button;
    }
    if (menu) {
        const SDL_Scancode extra[] = { SDL_SCANCODE_TAB, SDL_SCANCODE_T, SDL_SCANCODE_D, SDL_SCANCODE_ESCAPE };
        for (unsigned i = 0; i < SDL_arraysize(extra); i++) {
            SDL_Event event = { .key = { .type = SDL_EVENT_KEY_DOWN } };
            event.key.scancode = extra[i];
            if (pressed[extra[i]])
                keys |= PcUi_MenuEventKey(&event);
        }
    }
    return keys;
}

Uint16 PcUi_EventKey(const SDL_Event *event) {
    if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        float direction = PcSettings_Get()->invertWheel ? -event->wheel.y : event->wheel.y;
        return direction > 0 ? PAD_KEY_UP : direction < 0 ? PAD_KEY_DOWN : 0;
    }
    if (event->type != SDL_EVENT_KEY_DOWN || event->key.repeat)
        return 0;
    switch (event->key.scancode) {
    case SDL_SCANCODE_Z:
    case SDL_SCANCODE_RETURN:
        return PAD_BUTTON_A;
    case SDL_SCANCODE_X:
    case SDL_SCANCODE_ESCAPE:
        return PAD_BUTTON_B;
    case SDL_SCANCODE_UP:
        return PAD_KEY_UP;
    case SDL_SCANCODE_DOWN:
        return PAD_KEY_DOWN;
    case SDL_SCANCODE_LEFT:
        return PAD_KEY_LEFT;
    case SDL_SCANCODE_RIGHT:
        return PAD_KEY_RIGHT;
    case SDL_SCANCODE_Q:
        return PAD_BUTTON_L;
    case SDL_SCANCODE_W:
        return PAD_BUTTON_R;
    default:
        return 0;
    }
}

void PcUiInput_Sync(PcUiInput *input, const void *owner, unsigned phase) {
    if (input->owner != owner)
        *input = (PcUiInput){ .owner = owner, .phase = phase };
    else if (input->phase != phase) {
        input->phase = phase;
        input->pendingKeys = 0;
        input->action = (PcUiAction){ 0 };
        // A held page key must not become a fresh press merely because that page changed.
    }
}

bool PcUiInput_Event(PcUiInput *input, PcVideo *video, const SDL_Event *event) {
    input->pendingKeys |= PcUi_MenuEventKey(event);
    if (event->type == SDL_EVENT_KEY_DOWN)
        return true;
    if (event->type == SDL_EVENT_MOUSE_WHEEL)
        return true;
    if (event->type != SDL_EVENT_MOUSE_BUTTON_DOWN || event->button.button != SDL_BUTTON_LEFT)
        return false;
    PcUiAction action = PcUi_PointerAction(video, event, input->owner, input->phase);
    if (action.kind == PC_UI_ACTION_KEY)
        input->pendingKeys |= action.value;
    else if (action.kind != PC_UI_ACTION_NONE)
        input->action = action;
    return true;
}

Uint16 PcUiInput_Read(PcUiInput *input, Uint16 keys, Uint16 *pressed, PcUiAction *action) {
    keys |= input->pendingKeys;
    input->pendingKeys = 0;
    *pressed = keys & ~input->previousKeys;
    input->previousKeys = keys;
    *action = input->action;
    input->action = (PcUiAction){ 0 };
    return keys;
}
