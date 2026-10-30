#include "../ui_components.h"
#include "../ui_input.h"
#include "check.h"
#include "gfl/key.h"
#include <stdio.h>

static PcUiAction pointerAction;
static const void *pointerOwner;
static unsigned pointerPhase;
static bool keyboard[SDL_SCANCODE_COUNT];

const bool *SDL_GetKeyboardState(int *count) {
    if (count)
        *count = SDL_SCANCODE_COUNT;
    return keyboard;
}

PcUiAction PcUi_PointerAction(PcVideo *video, const SDL_Event *event, const void *owner, unsigned phase) {
    (void)video;
    (void)event;
    CHECK(owner == pointerOwner && phase == pointerPhase);
    return pointerAction;
}

int main(void) {
    int owner = 0, nextOwner = 0;
    keyboard[SDL_SCANCODE_ESCAPE] = true;
    CHECK(PcUi_KeyboardPad(false) & PAD_BUTTON_B);
    keyboard[SDL_SCANCODE_ESCAPE] = false;
    PcUiInput input = { 0 };
    PcUiInput_Sync(&input, &owner, 1);
    input.pendingKeys = PAD_BUTTON_A;
    input.action = (PcUiAction){ PC_UI_ACTION_ROW, 3 };
    PcUiInput_Sync(&input, &owner, 1);
    Uint16 pressed;
    PcUiAction action;
    CHECK(PcUiInput_Read(&input, PAD_BUTTON_R, &pressed, &action) == (PAD_BUTTON_A | PAD_BUTTON_R));
    CHECK(action.kind == PC_UI_ACTION_ROW && action.value == 3);
    CHECK(pressed == (PAD_BUTTON_A | PAD_BUTTON_R));
    CHECK(PcUiInput_Read(&input, PAD_BUTTON_R, &pressed, &action) == PAD_BUTTON_R);
    CHECK(!pressed && action.kind == PC_UI_ACTION_NONE);
    input.pendingKeys = PAD_BUTTON_A;
    input.action = (PcUiAction){ PC_UI_ACTION_ROW, 5 };
    PcUiInput_Sync(&input, &owner, 2);
    CHECK(PcUiInput_Read(&input, PAD_BUTTON_R, &pressed, &action) == PAD_BUTTON_R);
    CHECK(!pressed && action.kind == PC_UI_ACTION_NONE);
    input.pendingKeys = PAD_BUTTON_A;
    input.action = (PcUiAction){ PC_UI_ACTION_TAB, 1 };
    PcUiInput_Sync(&input, &nextOwner, 2);
    CHECK(PcUiInput_Read(&input, 0, &pressed, &action) == 0);
    CHECK(!pressed && action.kind == PC_UI_ACTION_NONE);

    SDL_Event event = { .key = { .type = SDL_EVENT_KEY_DOWN } };
    const struct {
        SDL_Scancode key;
        Uint16 button;
    } bindings[] = { { SDL_SCANCODE_RETURN, PAD_BUTTON_A }, { SDL_SCANCODE_Z, PAD_BUTTON_A },
                     { SDL_SCANCODE_ESCAPE, PAD_BUTTON_B }, { SDL_SCANCODE_X, PAD_BUTTON_B },
                     { SDL_SCANCODE_A, PAD_BUTTON_SELECT }, { SDL_SCANCODE_D, PAD_BUTTON_Y },
                     { SDL_SCANCODE_T, PAD_BUTTON_START },  { SDL_SCANCODE_Q, PAD_BUTTON_L },
                     { SDL_SCANCODE_W, PAD_BUTTON_R },      { SDL_SCANCODE_TAB, PAD_BUTTON_R } };
    for (unsigned i = 0; i < SDL_arraysize(bindings); i++) {
        event.key.scancode = bindings[i].key;
        CHECK(PcUi_MenuEventKey(&event) == bindings[i].button);
        event.key.repeat = true;
        CHECK(!PcUi_MenuEventKey(&event));
        event.key.repeat = false;
        keyboard[bindings[i].key] = true;
        CHECK(PcUi_KeyboardPad(true) == bindings[i].button);
        CHECK(PcUi_KeyboardPad(true) == bindings[i].button);
        keyboard[bindings[i].key] = false;
        CHECK(!PcUi_KeyboardPad(true));
    }
    event.type = SDL_EVENT_MOUSE_WHEEL;
    event.wheel.y = 0;
    CHECK(!PcUi_MenuEventKey(&event));
    event.wheel.y = 1;
    CHECK(PcUi_MenuEventKey(&event) == PAD_KEY_UP);

    event = (SDL_Event){ .button = { .type = SDL_EVENT_MOUSE_BUTTON_DOWN } };
    event.button.button = SDL_BUTTON_LEFT;
    pointerOwner = &nextOwner;
    pointerPhase = 2;
    pointerAction = (PcUiAction){ PC_UI_ACTION_ROW, 4 };
    CHECK(PcUiInput_Event(&input, NULL, &event));
    CHECK(!PcUiInput_Read(&input, 0, &pressed, &action));
    CHECK(action.kind == PC_UI_ACTION_ROW && action.value == 4);
    pointerAction = (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_A };
    CHECK(PcUiInput_Event(&input, NULL, &event));
    CHECK(PcUiInput_Read(&input, 0, &pressed, &action) == PAD_BUTTON_A);
    CHECK(pressed == PAD_BUTTON_A && action.kind == PC_UI_ACTION_NONE);
    event.button.button = SDL_BUTTON_RIGHT;
    CHECK(!PcUiInput_Event(&input, NULL, &event));
    event = (SDL_Event){ .key = { .type = SDL_EVENT_KEY_DOWN } };
    event.key.scancode = SDL_SCANCODE_RETURN;
    CHECK(PcUiInput_Event(&input, NULL, &event));
    CHECK(PcUiInput_Read(&input, 0, &pressed, &action) == PAD_BUTTON_A);
    CHECK(!pressed);
    CHECK(!PcUiInput_Read(&input, 0, &pressed, &action));
    puts("Menu input: owner/phase isolation, held-key edges and key bindings pass");
    return 0;
}
