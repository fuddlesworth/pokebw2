#ifndef POKEBW2_APP_MONOLITH_MONOLITH_TOOL_H
#define POKEBW2_APP_MONOLITH_MONOLITH_TOOL_H

#include "types.h"
#include "app/monolith/monolith_main.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "save/high_link.h"
#include "save/player_info.h"
#include "struct_decls.h"
#include "system/app_taskmenu.h"
#include "system/wordset.h"

// Fixed cursor and return-menu indices
#define MONOLITH_TEXT_CURSOR_NONE 0xff
#define MONOLITH_RETURN_POWER_MENU_CANCEL 3

// The Entralink monolith's helpers that its screens share (monolith_tool.c, the ROM's name): text shown as actors on
// a frame, the panels' palette effects, the return button, the yes/no and pass power menus, and the bar of the White
// and Black levels. The names are ours

// The widths of a text actor's frame
enum {
    MONOLITH_TEXT_WIDE,
    MONOLITH_TEXT_MEDIUM,
    MONOLITH_TEXT_NARROW,
};

// A line of text from the monolith's messages in a frame actor
typedef struct {
    ClActor *frame;
    BmpOamActor *text;
    GFLBitmap *bitmap;
    // Set until the text is printed and uploaded
    u8 printing;
} MonolithTextActor;

// The return button, which blinks when touched
typedef struct {
    ClActor *actor;
    u8 blinking;
    u8 timer;
    u8 lit;
} MonolithReturnButton;

void MonolithTool_CreateText(MonolithWork *wk, MonolithTextActor *text, int sub, int width, int x, int y, u32 msgId,
                             WordSet *wordSet);
void MonolithTool_CreateTextCentered(MonolithWork *wk, MonolithTextActor *text, int sub, int width, int x, int y,
                                     u32 msgId, WordSet *wordSet, u16 color);
void MonolithTool_DeleteText(MonolithTextActor *text);
void MonolithTool_SetTextVisible(MonolithTextActor *text, BOOL visible);
// Uploads the text once it is printed; returns whether it did now
BOOL MonolithTool_UpdateText(MonolithWork *wk, MonolithTextActor *text);
// Marks the text actor at cursor of count, with a pulse of the panel, or none for a cursor of 0xff
void MonolithTool_SetTextCursor(MonolithScreenParam *param, MonolithTextActor *texts, int count, int cursor, int panel);
// Marks the text actor at cursor of count as picked, with a flash of the panel
void MonolithTool_SetTextPicked(MonolithScreenParam *param, MonolithTextActor *texts, int count, int cursor, int panel);
void MonolithTool_SetPanelPulse(MonolithScreenParam *param, BOOL pulse, int panel);
void MonolithTool_FlashPanel(MonolithScreenParam *param, int panel);
void MonolithTool_InitPanels(MonolithScreenParam *param);
u8 MonolithTool_GetPanelMode(MonolithScreenParam *param, int panel);
void MonolithTool_UpdatePanel(MonolithScreenParam *param, int req);
PlayerInfo *MonolithTool_GetPlayerInfo(MonolithScreenParam *param);
PassPowerLevel *MonolithTool_GetLevels(MonolithScreenParam *param);
void MonolithTool_CreateReturnButton(MonolithWork *wk, MonolithReturnButton *button);
void MonolithTool_DeleteReturnButton(MonolithReturnButton *button);
void MonolithTool_UpdateReturnButton(MonolithReturnButton *button);
void MonolithTool_PressReturnButton(MonolithReturnButton *button);
BOOL MonolithTool_IsReturnButtonBlinking(MonolithReturnButton *button);
// An actor of the bottom screen, with the monolith's cells
ClActor *MonolithTool_CreateActor(MonolithWork *wk, s16 x, s16 y, u16 sequence);
void MonolithTool_DeleteActor(ClActor *actor);
void MonolithTool_UpdateActor(ClActor *actor);
AppTaskMenuRes *MonolithTool_CreateMenuRes(MonolithWork *wk, u32 bg, HeapID heapId);
void MonolithTool_FreeMenuRes(AppTaskMenuRes *res);
AppTaskMenu *MonolithTool_CreateYesNoMenu(MonolithWork *wk, AppTaskMenuRes *res, HeapID heapId);
void MonolithTool_FreeYesNoMenu(AppTaskMenu *menu);
// Returns whether an item was picked, and sets yes to whether it was YES
BOOL MonolithTool_UpdateYesNoMenu(MonolithWork *wk, int unused, AppTaskMenu *menu, BOOL *yes);
// The equipped pass powers and CANCEL, to pick one to give back
AppTaskMenu *MonolithTool_CreateReturnPowerMenu(MonolithScreenParam *param, MonolithWork *wk, AppTaskMenuRes *res,
                                                HeapID heapId);
void MonolithTool_FreeReturnPowerMenu(AppTaskMenu *menu);
// Returns whether an item was picked, and sets pos to it
BOOL MonolithTool_UpdateReturnPowerMenu(MonolithWork *wk, AppTaskMenu *menu, u32 *pos);
// Draws the bar of the White level against the Black level on row 14 of the top screen layout or 20 of the bottom's
void MonolithTool_DrawLevelBar(MonolithScreenParam *param, BOOL top, u32 bg);

#endif // POKEBW2_APP_MONOLITH_MONOLITH_TOOL_H
