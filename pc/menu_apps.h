#ifndef POKEBW2_PC_MENU_APPS_H
#define POKEBW2_PC_MENU_APPS_H
#include "gfl/msg.h"
#include "ui_components.h"

void PcMenu_Name(Font *font, MsgData *bank, u32 id, float x, float y, float right, u32 color, PcUiTextRole role);
void PcMenu_Paragraph(Font *font, MsgData *bank, u32 id, float x, float y, float right, float bottom, u32 color,
                      PcUiTextRole role);
void PcMenu_Actions(Font *font, AppTaskMenu *menu, u32 *start, PcUiRect rect, u32 rows);
#define PC_MENU_APP(name)                                                                                              \
    void PcMenu##name##_Reset(void);                                                                                   \
    bool PcMenu##name##_Draw(void);                                                                                    \
    Uint16 PcMenu##name##_FilterKeys(Uint16 keys);                                                                     \
    bool PcMenu##name##_HandleEvent(PcVideo *video, const SDL_Event *event)
PC_MENU_APP(Bag);
PC_MENU_APP(Summary);
PC_MENU_APP(Mail);
PC_MENU_APP(Phrase);
PC_MENU_APP(Map);
#undef PC_MENU_APP
#endif
