#include "menu_apps.h"
#include "gfl/key.h"
#include "gfl/ui.h"
#include "system/app_taskmenu.h"
#include "ui_widgets.h"

void PcMenu_Name(Font *font, MsgData *bank, u32 id, float x, float y, float right, u32 color, PcUiTextRole role) {
    u16 text[256];
    GFL_MsgDataLoadRawStr(bank, id, text, SDL_arraysize(text));
    PcUi_LineText(font, text, x, y, right, color, role);
}
void PcMenu_Paragraph(Font *font, MsgData *bank, u32 id, float x, float y, float right, float bottom, u32 color,
                      PcUiTextRole role) {
    u16 text[512];
    GFL_MsgDataLoadRawStr(bank, id, text, SDL_arraysize(text));
    PcUi_RoleParagraph(font, text, x, y, right, bottom, color, role);
}
void PcMenu_Actions(Font *font, AppTaskMenu *menu, u32 *start, PcUiRect rect, u32 rows) {
    AppTaskMenuNativeState state;
    if (!AppTaskMenu_GetNativeState(menu, &state))
        return;
    if (state.cursor < *start)
        *start = state.cursor;
    if (state.cursor >= *start + rows)
        *start = state.cursor - rows + 1;
    for (u32 row = 0; row < rows && *start + row < state.count; row++) {
        u32 i = *start + row;
        PcUi_Button(font, state.labels[i], PcUi_RowRect(rect, rect.height, row), i == state.cursor,
                    (PcUiAction){ state.ready ? PC_UI_ACTION_MENU : PC_UI_ACTION_NONE, i });
    }
}
