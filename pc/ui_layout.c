#include "ui_layout.h"

PcUiRect PcUi_RowRect(PcUiRect first, float stride, unsigned row) {
    first.y += stride * row;
    return first;
}

bool PcUi_Contains(PcUiShape shape, float x, float y) {
    float left = x - shape.rect.x, top = y - shape.rect.y;
    float right = shape.rect.width - left, bottom = shape.rect.height - top;
    return left >= 0 && top >= 0 && right > 0 && bottom > 0 && left + top >= shape.topLeft &&
           right + top >= shape.topRight && right + bottom >= shape.bottomRight && left + bottom >= shape.bottomLeft;
}

void PcUiControls_Begin(PcUiControls *controls, const void *owner, unsigned phase) {
    controls->owner = owner;
    controls->phase = phase;
    controls->count = 0;
}

bool PcUiControls_Add(PcUiControls *controls, PcUiShape shape, PcUiAction action) {
    if (!controls->owner || action.kind == PC_UI_ACTION_NONE || shape.rect.width <= 0 || shape.rect.height <= 0 ||
        controls->count == sizeof(controls->controls) / sizeof(controls->controls[0]))
        return false;
    controls->controls[controls->count++] = (PcUiControl){ shape, action };
    return true;
}

PcUiAction PcUiControls_Hit(const PcUiControls *controls, const void *owner, unsigned phase, float x, float y) {
    if (owner && controls->owner == owner && controls->phase == phase) {
        /* Later controls overlay earlier ones, just as their drawing does. */
        for (size_t i = controls->count; i > 0; i--)
            if (PcUi_Contains(controls->controls[i - 1].shape, x, y))
                return controls->controls[i - 1].action;
    }
    return (PcUiAction){ PC_UI_ACTION_NONE, 0 };
}
