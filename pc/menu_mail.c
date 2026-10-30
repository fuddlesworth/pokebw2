#include "field/mailsys.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/str.h"
#include "menu_apps.h"
#include "system/app_taskmenu.h"
#include "system/pms_word.h"
#include "ui_frame.h"
#include "ui_widgets.h"

static Font *font;
static PcUiInput input;
static u32 actionStart;
static void DrawMail(MailSysWork *wk) {
    font = wk->font;
    bool write = wk->initialPhase == MAIL_PHASE_WRITE;
    PcUiFrame_Draw(font, PC_MENU_MAIL, "MAIL", write ? "WRITE" : "READ",
                   write ? "Up/Down: Choose a line   Z: Edit   T: Finish" : "Z or X: Return to Pokémon");
    PcUi_Tab(font, write ? "COMPOSE" : "MESSAGE", (PcUiRect){ 28, 101, 203, 43 }, true, (PcUiAction){ 0 });
    PcUi_LineLabel(font, "From", 65, 181, 200, PC_UI_INK_MUTED, PC_UI_TEXT_BODY);
    PcUi_LineText(font, GFL_StrBufGetStringPtr(wk->edit->name), 167, 178, 801, PC_UI_INK, PC_UI_TEXT_CARD_TITLE);
    for (u32 i = 0; i < 3; i++) {
        float y = 244 + i * 91;
        if (write)
            PcUi_SkinControl(PC_MENU_LIGHT_ROW, (PcUiRect){ 59, y, 758, 76 },
                             wk->cursor == i && wk->phase == MAIL_PHASE_WRITE,
                             (PcUiAction){ wk->phase == MAIL_PHASE_WRITE ? PC_UI_ACTION_ROW : PC_UI_ACTION_NONE, i });
        if (PMSData_IsNotEmpty(&wk->edit->words[i])) {
            StrBuf *text = PMSData_ToString(&wk->edit->words[i], HEAPID_USER);
            PcUi_RoleParagraph(font, GFL_StrBufGetStringPtr(text), 78, y + 13, 791, y + 73, PC_UI_INK, PC_UI_TEXT_BODY);
            GFL_StrBufFree(text);
        } else
            PcUi_LineLabel(font, write ? "Choose a phrase…" : "—", 78, y + 18, 791, PC_UI_INK_MUTED, PC_UI_TEXT_BODY);
    }
    PcUi_LineLabel(font, "Word", 78, 552, 200, PC_UI_INK_MUTED, PC_UI_TEXT_BODY);
    if (write)
        PcUi_SkinControl(PC_MENU_LIGHT_ROW, (PcUiRect){ 216, 541, 600, 50 },
                         wk->cursor == 3 && wk->phase == MAIL_PHASE_WRITE,
                         (PcUiAction){ wk->phase == MAIL_PHASE_WRITE ? PC_UI_ACTION_ROW : PC_UI_ACTION_NONE, 3 });
    StrBuf *word = GFL_StrBufCreate(64, HEAPID_USER);
    loadSayingToString(wk->edit->word, word, HEAPID_USER);
    if (wk->edit->word == PMS_WORD_NULL)
        PcUi_LineLabel(font, write ? "Choose a word…" : "—", 237, 553, 790, PC_UI_INK, PC_UI_TEXT_BODY);
    else
        PcUi_LineText(font, GFL_StrBufGetStringPtr(word), 237, 553, 790, PC_UI_INK, PC_UI_TEXT_BODY);
    GFL_StrBufFree(word);
    if (wk->phase > MAIL_PHASE_WRITE) {
        PcUi_RoleParagraph(font, wk->nativeMessage, 906, 192, 1225, 437, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        if (wk->menu)
            PcMenu_Actions(font, wk->menu, &actionStart, (PcUiRect){ 902, 492, 322, 36 }, 2);
        else
            PcUi_LineLabel(font, "Z: Continue", 906, 560, 1225, PC_UI_CYAN, PC_UI_TEXT_BODY);
    } else if (write) {
        PcUi_LineLabel(font, "YOUR MESSAGE", 906, 183, 1225, PC_UI_CYAN, PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "Choose three phrases", 906, 235, 1225, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "and a word for this Mail.", 906, 267, 1225, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "The Mail is attached", 906, 331, 1225, PC_UI_MUTED, PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "when you finish writing.", 906, 363, 1225, PC_UI_MUTED, PC_UI_TEXT_BODY);
        const char *buttons[] = { "Finish writing", "Cancel" };
        for (u32 i = 0; i < 2; i++) {
            PcUi_ButtonLabel(font, buttons[i], (PcUiRect){ 902, 492 + i * 48, 322, 36 }, wk->cursor == i + 4,
                             (PcUiAction){ PC_UI_ACTION_ROW, i + 4 });
        }
    } else {
        PcUi_LineLabel(font, "POKÉMON MAIL", 906, 183, 1225, PC_UI_CYAN, PC_UI_TEXT_BODY);
        PcUi_LineLabel(font, "A message from", 906, 246, 1225, PC_UI_DEFAULT_COLOR, PC_UI_TEXT_BODY);
        PcUi_LineText(font, GFL_StrBufGetStringPtr(wk->edit->name), 906, 282, 1225, PC_UI_DEFAULT_COLOR,
                      PC_UI_TEXT_VALUE);
        PcUi_LineLabel(font, "Z: Return", 906, 560, 1225, PC_UI_MUTED, PC_UI_TEXT_BODY);
    }
}

static void Sync(MailSysWork *work) {
    PcUiInput_Sync(&input, work, work->phase);
}
void PcMenuMail_Reset(void) {
    input = (PcUiInput){ 0 };
    actionStart = 0;
}
bool PcMenuMail_Draw(void) {
    MailSysWork *work = MailSys_GetNativeWork();
    if (!work)
        return false;
    Sync(work);
    PcUi_BeginControls(input.owner, input.phase);
    if (work->initialPhase == MAIL_PHASE_VIEW || (work->phase > MAIL_PHASE_WRITE && !work->menu))
        PcUi_Control((PcUiShape){ .rect = { 26, 145, 1228, 500 } }, (PcUiAction){ PC_UI_ACTION_KEY, PAD_BUTTON_A });
    DrawMail(work);
    return true;
}
bool PcMenuMail_HandleEvent(PcVideo *video, const SDL_Event *event) {
    MailSysWork *work = MailSys_GetNativeWork();
    if (!work)
        return false;
    Sync(work);
    return PcUiInput_Event(&input, video, event);
}
Uint16 PcMenuMail_FilterKeys(Uint16 keys) {
    MailSysWork *work = MailSys_GetNativeWork();
    if (!work)
        return keys;
    Sync(work);
    Uint16 pressed;
    PcUiAction action;
    keys = PcUiInput_Read(&input, keys, &pressed, &action);
    if (action.kind == PC_UI_ACTION_ROW && MailSys_SetNativeCursor(action.value))
        keys |= PAD_BUTTON_A;
    if (action.kind == PC_UI_ACTION_MENU) {
        AppTaskMenuNativeState state;
        if (AppTaskMenu_GetNativeState(work->menu, &state) && state.ready && (u32)action.value < state.count) {
            AppTaskMenu_SetCursorPos(work->menu, action.value);
            keys |= PAD_BUTTON_A;
        }
    }
    return keys;
}
