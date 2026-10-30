#include "app/pms_input.h"
#include "app/pms_select.h"
#include "app/pmsi_initial_data.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/str.h"
#include "gfl/ui.h"
#include "menu_apps.h"
#include "system/pmsi_param.h"
#include "ui_frame.h"
#include "ui_widgets.h"

static Font *font;
static PcUiInput input;
static u32 wordStart;
static void DrawPhrase(PMSInputNativeState *state) {
    font = state->font;
    PcUiFrame_Draw(font, PC_MENU_MAIL, PMSSelect_IsNativeRunning() ? "TRAINER" : "MAIL", "PHRASES",
                   "Arrows: Browse   A: Categories/search   Q/W: Word pages");
    PcUi_Tab(font,
             state->phase == PMS_NATIVE_EDIT         ? "EDIT PHRASE"
             : state->phase == PMS_NATIVE_CATEGORIES ? "CATEGORIES"
                                                     : "WORDS",
             (PcUiRect){ 28, 101, 203, 43 }, true, (PcUiAction){ 0 });
    PcUi_LineLabel(font, state->mode == PMSI_MODE_SENTENCE ? "MESSAGE PREVIEW" : "CHOOSE A WORD", 65, 177, 815,
                   PC_UI_DEFAULT_COLOR, PC_UI_TEXT_SECTION);
    if (state->mode == PMSI_MODE_SENTENCE)
        PcUi_RoleParagraph(font, state->preview, 65, 220, 815, 304, PC_UI_INK, PC_UI_TEXT_BODY);
    else
        PcUi_LineText(font, state->words[0], 65, 222, 815, PC_UI_INK, PC_UI_TEXT_CARD_TITLE);
    StrBuf *label = GFL_StrBufCreate(256, HEAPID_USER);
    if (state->phase == PMS_NATIVE_EDIT) {
        for (u32 i = 0; i < state->editCount && i < 2; i++) {
            float y = 335 + i * 65;
            PcUi_SkinControl(PC_MENU_LIGHT_ROW, (PcUiRect){ 59, y, 758, 50 },
                             !state->buttonFocused && state->editCursor == i, (PcUiAction){ PC_UI_ACTION_ROW, i });
            char title[32];
            SDL_snprintf(title, sizeof(title), "Word %u", i + 1);
            PcUi_LineLabel(font, title, 77, y + 10, 240, PC_UI_INK, PC_UI_TEXT_BODY);
            if (state->words[i][0] == 0xffff)
                PcUi_LineLabel(font, "Choose a word…", 264, y + 10, 790, PC_UI_INK, PC_UI_TEXT_BODY);
            else
                PcUi_LineText(font, state->words[i], 264, y + 10, 790, PC_UI_INK, PC_UI_TEXT_BODY);
        }
        if (state->mode == PMSI_MODE_SENTENCE && !state->fixedSentence) {
            PcUi_LineLabel(font, "Left / Right: Change the sentence", 77, 517, 810, PC_UI_INK, PC_UI_TEXT_BODY);
        }
        PcUi_LineLabel(font, state->complete ? "READY TO USE" : "FILL IN THE WORDS", 905, 182, 1220, PC_UI_CYAN,
                       PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "Choose a highlighted", 905, 237, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "word to edit it.", 905, 270, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        const char *buttons[] = { "Use this phrase", "Cancel" };
        for (u32 i = 0; i < 2; i++) {
            bool selected = state->buttonFocused && state->button == i;
            PcUi_ButtonLabel(font, buttons[i], (PcUiRect){ 902, 492 + i * 48, 322, 36 }, selected,
                             (PcUiAction){ PC_UI_ACTION_CUSTOM, i });
        }
    } else if (state->phase == PMS_NATIVE_CATEGORIES) {
        u32 count = state->categoryMode ? PMSIInitial_GetCount() : 12;
        u32 columns = state->categoryMode ? 7 : 3;
        float width = state->categoryMode ? 100 : 246;
        for (u32 i = 0; i < count; i++) {
            float x = 59 + (i % columns) * (width + 8), y = 322 + (i / columns) * 55;
            bool enabled = PMSInput_GetNativeCategory(i, label);
            PcUi_SkinControl(PC_MENU_LIGHT_ROW, (PcUiRect){ x, y, width, 46 }, state->category == i,
                             (PcUiAction){ enabled ? PC_UI_ACTION_ROW : PC_UI_ACTION_NONE, i });
            PcUi_RoleCenteredText(font, GFL_StrBufGetStringPtr(label), x + 5, y + 10, x + width - 12, y + 45,
                                  enabled ? PC_UI_INK : 0x7a8994, PC_UI_TEXT_STAT_VALUE);
        }
        PcUi_LineLabel(font, state->categoryMode ? "SEARCH WORDS" : "WORD CATEGORIES", 905, 182, 1220, PC_UI_CYAN,
                       PC_UI_TEXT_BODY);
        if (state->categoryMode) {
            PMSInput_GetSearchInputStr(PMSInput_GetNativeWork(), label);
            PcUi_LineText(font, GFL_StrBufGetStringPtr(label), 905, 237, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_VALUE);
            const char *buttons[] = { "Search", "Erase letter", "Back" };
            for (u32 i = 0; i < 3; i++) {
                bool selected = state->category == 0xfc + i;
                PcUi_ButtonLabel(font, buttons[i], (PcUiRect){ 902, 420 + i * 48, 322, 36 }, selected,
                                 (PcUiAction){ PC_UI_ACTION_ROW, 0xfc + i });
            }
        } else {
            PcUi_LineLabel(font, "A: Search by letter", 905, 237, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
            PcUi_LineLabel(font, "X: Return to phrase", 905, 285, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        }
    } else {
        if (state->cursor < state->count) {
            if (state->cursor < wordStart)
                wordStart = state->cursor;
            if (state->cursor >= wordStart + 12)
                wordStart = state->cursor - 10;
        }
        wordStart &= ~1u;
        for (u32 row = 0; row < 12 && wordStart + row < state->count; row++) {
            u32 index = wordStart + row;
            float x = 59 + (row % 2) * 383, y = 310 + (row / 2) * 48;
            PMSInput_GetCategoryWord(PMSInput_GetNativeWork(), index, label);
            PcUi_SkinControl(PC_MENU_LIGHT_ROW, (PcUiRect){ x, y, 375, 44 }, state->cursor == index,
                             (PcUiAction){ PC_UI_ACTION_ROW, index });
            PcUi_LineText(font, GFL_StrBufGetStringPtr(label), x + 15, y + 9, x + 355, PC_UI_INK, PC_UI_TEXT_BODY);
        }
        char count[64];
        SDL_snprintf(count, sizeof(count), "%u %s", state->count, state->count == 1 ? "WORD" : "WORDS");
        PcUi_LineLabel(font, count, 905, 182, 1220, PC_UI_CYAN, PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "Z: Use selected word", 905, 246, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "X: Back to categories", 905, 286, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "Q/W: Previous/next page", 905, 326, 1220, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_LABEL);
    }
    GFL_StrBufFree(label);
}

static void Sync(const PMSInputNativeState *state) {
    PcUiInput_Sync(&input, PMSInput_GetNativeWork(), state->phase | (state->categoryMode << 8));
}
void PcMenuPhrase_Reset(void) {
    input = (PcUiInput){ 0 };
    wordStart = 0;
}
bool PcMenuPhrase_Draw(void) {
    PMSInputNativeState state;
    if (!PMSInput_GetNativeState(&state))
        return false;
    Sync(&state);
    PcUi_BeginControls(input.owner, input.phase);
    DrawPhrase(&state);
    return true;
}
bool PcMenuPhrase_HandleEvent(PcVideo *video, const SDL_Event *event) {
    PMSInputNativeState state;
    if (!PMSInput_GetNativeState(&state))
        return false;
    Sync(&state);
    return PcUiInput_Event(&input, video, event);
}
Uint16 PcMenuPhrase_FilterKeys(Uint16 keys) {
    PMSInputNativeState state;
    if (!PMSInput_GetNativeState(&state))
        return keys;
    Sync(&state);
    Uint16 pressed;
    PcUiAction action;
    keys = PcUiInput_Read(&input, keys, &pressed, &action);
    if (action.kind == PC_UI_ACTION_ROW && PMSInput_SelectNative(action.value))
        keys |= PAD_BUTTON_A;
    if (action.kind == PC_UI_ACTION_CUSTOM)
        PMSInput_NativeConfirm(action.value != 0);
    GCTX_HIDSetUsingTouch(FALSE);
    return keys;
}
