#include "ui_components.h"
#include "gfl/key.h"
#include "ui_widgets.h"

static PcUiControls controls;

void PcUi_BeginControls(const void *owner, unsigned phase) {
    PcUiControls_Begin(&controls, owner, phase);
}

void PcUi_Control(PcUiShape shape, PcUiAction action) {
    PcUiControls_Add(&controls, shape, action);
}

PcUiAction PcUi_PointerAction(PcVideo *video, const SDL_Event *event, const void *owner, unsigned phase) {
    float x, y;
    if (event->type != SDL_EVENT_MOUSE_BUTTON_DOWN || event->button.button != SDL_BUTTON_LEFT ||
        !PcVideo_MapPointer(video, event->button.x, event->button.y, &x, &y))
        return (PcUiAction){ PC_UI_ACTION_NONE, 0 };
    return PcUiControls_Hit(&controls, owner, phase, x * PC_UI_WIDTH / PC_CANVAS_WIDTH,
                            y * PC_UI_HEIGHT / PC_CANVAS_HEIGHT);
}

PcUiShape PcUi_SkinShape(PcMenuSkinAsset asset, PcUiRect rect) {
    PcUiShape shape = { .rect = rect };
    PcMenuSkinGeometry geometry = PcMenuSkin_GetGeometry(asset);
    if (geometry.height > 0) {
        float scale = rect.height / geometry.height;
        shape.topLeft = geometry.topLeft * scale;
        shape.topRight = geometry.topRight * scale;
        shape.bottomRight = geometry.bottomRight * scale;
        shape.bottomLeft = geometry.bottomLeft * scale;
    }
    return shape;
}

void PcUi_SkinControl(PcMenuSkinAsset asset, PcUiRect rect, unsigned state, PcUiAction action) {
    PcMenuSkin_Add(asset, rect.x, rect.y, rect.width, rect.height, state);
    PcUi_Control(PcUi_SkinShape(asset, rect), action);
}

void PcUi_TabLabel(Font *font, const char *label, PcUiShape shape, bool selected) {
    // At half height, the two sloping edges define the tab's visible center.
    float left = shape.rect.x + (shape.topLeft + shape.bottomLeft) / 2;
    float right = shape.rect.x + shape.rect.width - (shape.topRight + shape.bottomRight) / 2;
    PcUi_RoleCenteredLabel(font, label, left + PC_UI_SPACE_XS, shape.rect.y + PC_UI_SPACE_SM, right - PC_UI_SPACE_XS,
                           shape.rect.y + shape.rect.height, selected ? PC_UI_INK : PC_UI_MUTED, PC_UI_TEXT_TAB);
}

void PcUi_Tab(Font *font, const char *label, PcUiRect rect, bool selected, PcUiAction action) {
    PcUi_SkinControl(PC_MENU_TAB, rect, selected, action);
    PcUi_TabLabel(font, label, PcUi_SkinShape(PC_MENU_TAB, rect), selected);
}

void PcUi_Button(Font *font, const u16 *label, PcUiRect rect, bool selected, PcUiAction action) {
    PcUi_SkinControl(PC_MENU_BUTTON, rect, selected, action);
    PcUi_RoleText(font, label, rect.x + PC_UI_SPACE_LG, rect.y + PC_UI_CONTROL_TEXT_TOP,
                  rect.x + rect.width - PC_UI_SPACE_LG - PC_UI_SPACE_XS / 2, rect.y + rect.height,
                  selected ? PC_UI_INK : PC_UI_WHITE, PC_UI_TEXT_BUTTON);
}

void PcUi_ButtonLabel(Font *font, const char *label, PcUiRect rect, bool selected, PcUiAction action) {
    PcUi_SkinControl(PC_MENU_BUTTON, rect, selected, action);
    PcUi_RoleLabel(font, label, rect.x + PC_UI_SPACE_LG, rect.y + PC_UI_CONTROL_TEXT_TOP,
                   rect.x + rect.width - PC_UI_SPACE_LG - PC_UI_SPACE_XS / 2, rect.y + rect.height,
                   selected ? PC_UI_INK : PC_UI_WHITE, PC_UI_TEXT_BUTTON);
}

void PcUi_CenteredButtonLabel(Font *font, const char *label, PcUiRect rect, bool selected, bool enabled,
                              PcUiAction action) {
    PcUi_SkinControl(PC_MENU_BUTTON, rect, selected && enabled,
                     enabled ? action : (PcUiAction){ PC_UI_ACTION_NONE, 0 });
    PcUi_RoleCenteredLabel(font, label, rect.x + PC_UI_SPACE_XS, rect.y + PC_UI_CONTROL_TEXT_TOP,
                           rect.x + rect.width - PC_UI_SPACE_MD, rect.y + rect.height,
                           !enabled   ? PC_UI_MUTED
                           : selected ? PC_UI_INK
                                      : PC_UI_WHITE,
                           PC_UI_TEXT_BUTTON);
}
