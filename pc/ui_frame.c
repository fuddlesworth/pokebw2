#include "ui_frame.h"
#include "gfl/key.h"
#include "ui_components.h"
#include "ui_widgets.h"

float PcUiFrame_KeyHint(Font *font, const char *key, const char *label, float right, float centerY, float keyWidth,
                        u16 button) {
    const PcUiTextStyle *style = PcUi_Style(PC_UI_TEXT_KEY_HINT);
    float width = PcUi_RoleLabelWidth(font, label, PC_UI_TEXT_KEY_HINT);
    float x = right - width - PC_UI_KEY_GAP - keyWidth;
    float top = centerY - PC_UI_KEY_HEIGHT / 2;
    float textTop = centerY - PC_UI_FONT_CELL / 2 * style->scale;
    PcMenuSkin_Add(PC_BATTLE_KEY, x, top, keyWidth, PC_UI_KEY_HEIGHT, 0);
    PcUi_RoleCenteredLabel(font, key, x + PC_UI_SPACE_XS, textTop, x + keyWidth - PC_UI_SPACE_XS,
                           top + PC_UI_KEY_HEIGHT, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_KEY_HINT);
    PcUi_RoleLabel(font, label, x + keyWidth + PC_UI_KEY_GAP, textTop, right + 2, top + PC_UI_KEY_HEIGHT,
                   PC_UI_DEFAULT_COLOR, PC_UI_TEXT_KEY_HINT);
    PcUi_Control((PcUiShape){ .rect = { x, top, right - x, PC_UI_KEY_HEIGHT } },
                 (PcUiAction){ PC_UI_ACTION_KEY, button });
    return x;
}

float PcUiFrame_DirectionHint(Font *font, const char *label, float right, float centerY) {
    static const u16 buttons[] = { PAD_KEY_UP, PAD_KEY_DOWN, PAD_KEY_LEFT, PAD_KEY_RIGHT };
    float groupWidth = 3 * PC_UI_KEY_STEP + PC_UI_KEY_WIDTH;
    float width = PcUi_RoleLabelWidth(font, label, PC_UI_TEXT_KEY_HINT);
    float x = right - width - PC_UI_KEY_GAP - groupWidth;
    float top = centerY - PC_UI_KEY_HEIGHT / 2;
    for (u32 i = 0; i < SDL_arraysize(buttons); i++) {
        PcUi_SkinControl(PC_BATTLE_KEY, (PcUiRect){ x + i * PC_UI_KEY_STEP, top, PC_UI_KEY_WIDTH, PC_UI_KEY_HEIGHT },
                         i + 1, (PcUiAction){ PC_UI_ACTION_KEY, buttons[i] });
    }
    PcUi_RoleLabel(font, label, x + groupWidth + PC_UI_KEY_GAP,
                   centerY - PC_UI_FONT_CELL / 2 * PcUi_Style(PC_UI_TEXT_KEY_HINT)->scale, right + 2,
                   top + PC_UI_KEY_HEIGHT, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_KEY_HINT);
    return x;
}

void PcUiFrame_Header(Font *font, const char *title, const char *section) {
    PcUiRect rect = PcUi_HeaderTitleBounds;
    PcUi_RoleLabel(font, title, rect.x, rect.y, rect.x + rect.width, rect.y + rect.height, PC_UI_DEFAULT_COLOR,
                   PC_UI_TEXT_SCREEN_TITLE);
    if (section) {
        rect = PcUi_HeaderSectionBounds;
        PcUi_RoleCenteredLabel(font, section, rect.x, rect.y, rect.x + rect.width, rect.y + rect.height,
                               PC_UI_DEFAULT_COLOR, PC_UI_TEXT_SECTION);
    }
}

void PcUiFrame_Footer(Font *font, const char *hint, const char *action, bool browse, bool enabled) {
    u32 color = enabled ? PC_UI_WHITE : PC_UI_DISABLED;
    PcUi_RoleLabel(font, hint, 26, 682, browse ? 810 : 1000, 714, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_LABEL);
    if (browse)
        PcUi_RoleLabel(font, "Browse", 914, 680, 1005, 712, color, PC_UI_TEXT_FOOTER);
    PcUi_RoleCenteredLabel(font, "Z", 1025, 677, 1052, 712, color, PC_UI_TEXT_FOOTER);
    PcUi_RoleCenteredLabel(font, "X", 1162, 677, 1189, 712, color, PC_UI_TEXT_FOOTER);
    PcUi_RoleLabel(font, action, 1060, 680, 1150, 712, color, PC_UI_TEXT_FOOTER);
    PcUi_RoleLabel(font, "Back", 1200, 680, 1270, 712, color, PC_UI_TEXT_FOOTER);
    if (enabled) {
        PcUi_Control((PcUiShape){ .rect = { 1020, 674, 132, 40 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_A });
        PcUi_Control((PcUiShape){ .rect = { 1158, 674, 116, 40 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_B });
        if (browse) {
            PcUi_Control((PcUiShape){ .rect = { 835, 674, 35, 40 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_KEY_UP });
            PcUi_Control((PcUiShape){ .rect = { 873, 674, 30, 40 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_KEY_DOWN });
        }
    }
}

void PcUiFrame_Draw(Font *font, PcMenuSkinAsset body, const char *title, const char *section, const char *hint) {
    PcMenuSkin_Add(PC_MENU_HEADER, 0, 0, PC_UI_WIDTH, PC_UI_HEADER_HEIGHT, 0);
    PcMenuSkin_Add(body, 0, PC_UI_HEADER_HEIGHT, PC_UI_WIDTH, PC_UI_BODY_HEIGHT, 0);
    PcMenuSkin_Add(PC_MENU_FOOTER, 0, PC_UI_FOOTER_TOP, PC_UI_WIDTH, PC_UI_FOOTER_HEIGHT, 0);
    PcUiFrame_Header(font, title, section);
    PcUiFrame_Footer(font, hint, "Select", false, true);
}
