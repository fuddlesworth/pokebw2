#ifndef POKEBW2_PC_UI_THEME_H
#define POKEBW2_PC_UI_THEME_H

#include "ui_draw.h"
#include "ui_layout.h"

enum {
    PC_UI_INK = 0x202b32,
    PC_UI_WHITE = 0xe6edf1,
    PC_UI_MUTED = 0x9cabb5,
    PC_UI_INK_MUTED = 0x5b6b77,
    PC_UI_DISABLED = 0x586670,
    PC_UI_CYAN = 0x02d5e8,
    PC_UI_LINK = 0x007c89,
    PC_UI_WARNING = 0xffca55,
    PC_UI_DANGER = 0xff5266,
    PC_UI_SUCCESS = 0x77e4a1,
    PC_UI_SPACE_XS = 4,
    PC_UI_SPACE_SM = 8,
    PC_UI_SPACE_MD = 12,
    PC_UI_SPACE_LG = 16,
    PC_UI_SPACE_XL = 24,
    PC_UI_HEADER_HEIGHT = 81,
    PC_UI_FOOTER_TOP = 646,
    PC_UI_FOOTER_HEIGHT = PC_UI_HEIGHT - PC_UI_FOOTER_TOP,
    PC_UI_BODY_HEIGHT = PC_UI_FOOTER_TOP - PC_UI_HEADER_HEIGHT,
    PC_UI_FONT_CELL = 16,
    PC_UI_FONT_BASELINE = 12,
    PC_UI_CONTROL_TEXT_TOP = 5,
    PC_UI_KEY_HEIGHT = 28,
    PC_UI_KEY_WIDTH = 26,
    PC_UI_KEY_STEP = 31,
    PC_UI_KEY_GROUP_GAP = 26,
    PC_UI_FOOTER_KEY_CENTER = 685,
    PC_UI_KEY_GAP = PC_UI_SPACE_MD
};
#define PC_UI_DEFAULT_COLOR 0xffffffffu

typedef enum {
    PC_UI_TEXT_SCREEN_TITLE,
    PC_UI_TEXT_SECTION,
    PC_UI_TEXT_PANEL_TITLE,
    PC_UI_TEXT_BUTTON,
    PC_UI_TEXT_FOOTER,
    PC_UI_TEXT_CARD_TITLE,
    PC_UI_TEXT_TAB,
    PC_UI_TEXT_BODY,
    PC_UI_TEXT_LABEL,
    PC_UI_TEXT_CAPTION,
    PC_UI_TEXT_VALUE,
    PC_UI_TEXT_HERO_VALUE,
    PC_UI_TEXT_KEY_HINT,
    PC_UI_TEXT_STAT_LABEL,
    PC_UI_TEXT_STAT_VALUE,
    PC_UI_TEXT_ROLE_COUNT
} PcUiTextRole;

typedef struct {
    PcUiFont font;
    float scale;
    float lineHeight;
    u32 color;
} PcUiTextStyle;

const PcUiTextStyle *PcUi_Style(PcUiTextRole role);
extern const PcUiRect PcUi_HeaderTitleBounds;
extern const PcUiRect PcUi_HeaderSectionBounds;

#endif
