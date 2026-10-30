#include "ui_theme.h"

// Roles are shared across screens; fitting to a component's bounds never increases these nominal sizes.
static const PcUiTextStyle textStyles[PC_UI_TEXT_ROLE_COUNT] = {
    [PC_UI_TEXT_SCREEN_TITLE] = { PC_UI_FONT_HEADING, 2.4f, 17, PC_UI_WHITE },
    [PC_UI_TEXT_SECTION] = { PC_UI_FONT_BOLD, 2.0f, 17, PC_UI_INK },
    [PC_UI_TEXT_PANEL_TITLE] = { PC_UI_FONT_HEADING, 2.3f, 17, PC_UI_INK },
    [PC_UI_TEXT_BUTTON] = { PC_UI_FONT_BOLD, 1.8f, 17, PC_UI_WHITE },
    [PC_UI_TEXT_FOOTER] = { PC_UI_FONT_BOLD, 1.7f, 17, PC_UI_WHITE },
    [PC_UI_TEXT_CARD_TITLE] = { PC_UI_FONT_BOLD, 2.3f, 17, PC_UI_WHITE },
    [PC_UI_TEXT_TAB] = { PC_UI_FONT_BOLD, 1.9f, 17, PC_UI_MUTED },
    [PC_UI_TEXT_BODY] = { PC_UI_FONT_BOLD, 1.8f, 17, PC_UI_WHITE },
    [PC_UI_TEXT_LABEL] = { PC_UI_FONT_BOLD, 1.6f, 17, PC_UI_MUTED },
    [PC_UI_TEXT_CAPTION] = { PC_UI_FONT_BOLD, 1.3f, 17, PC_UI_MUTED },
    [PC_UI_TEXT_VALUE] = { PC_UI_FONT_BOLD, 2.0f, 17, PC_UI_WHITE },
    [PC_UI_TEXT_HERO_VALUE] = { PC_UI_FONT_BOLD, 2.8f, 17, PC_UI_WHITE },
    [PC_UI_TEXT_KEY_HINT] = { PC_UI_FONT_BOLD, 1.1f, 17, PC_UI_WHITE },
    [PC_UI_TEXT_STAT_LABEL] = { PC_UI_FONT_BOLD, 1.5f, 17, PC_UI_INK_MUTED },
    [PC_UI_TEXT_STAT_VALUE] = { PC_UI_FONT_BOLD, 1.7f, 17, PC_UI_INK },
};

const PcUiRect PcUi_HeaderTitleBounds = { 118, 24, 251, 49 };
const PcUiRect PcUi_HeaderSectionBounds = { 443, 32, 180, 41 };

const PcUiTextStyle *PcUi_Style(PcUiTextRole role) {
    if ((unsigned)role >= PC_UI_TEXT_ROLE_COUNT)
        role = PC_UI_TEXT_BODY;
    return &textStyles[role];
}
