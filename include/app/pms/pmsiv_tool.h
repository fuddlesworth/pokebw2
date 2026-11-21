#ifndef POKEBW2_APP_PMS_PMSIV_TOOL_H
#define POKEBW2_APP_PMS_PMSIV_TOOL_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"

// The phrase input's BG effects that the categories and the word window share: a BG scrolled over some frames, and
// an alpha blend or a brightness changed over some frames, each run by a V-blank task. The ROM doesn't name this file;
// pmsiv_tool.c is a guess after its neighbors. The names are ours, guessed

// Where a cursor goes to be out of sight. It sits between pmsiv_category.c's data and pmsiv_menu.c's, so it is
// placed here
extern const ClActorPos PMSIV_CURSOR_HIDE_POS;

// The direction of a scroll
#define PMSIV_TOOL_SCROLL_X 0
#define PMSIV_TOOL_SCROLL_Y 1

// The largest blend, and the largest brightness
#define PMSIV_BLEND_MAX 124
#define PMSIV_BRIGHT_MAX 16

typedef struct {
    int plane1;
    int plane2;
    int ev;
    int evEnd;
    int evStep;
    int wait;
    int seq;
    TCB *tcb;
} PMSIVToolBlendWork;

typedef struct {
    u32 bg;
    u32 op;
    fx32 pos;
    fx32 endPos;
    fx32 step;
    u16 timer;
    u16 seq;
    TCB *tcb;
} PMSIVToolScrollWork;

// Scrolls a BG by vector pixels over wait frames, once PMSIVTool_WaitScroll is first called
void PMSIVTool_SetupScrollWork(PMSIVToolScrollWork *wk, u32 bg, int direction, int vector, int wait);
BOOL PMSIVTool_WaitScroll(PMSIVToolScrollWork *wk);
void PMSIVTool_SetupBlendWork(PMSIVToolBlendWork *wk, int plane1, int plane2, int start, int end, int wait);
BOOL PMSIVTool_WaitBlend(PMSIVToolBlendWork *wk);
void PMSIVTool_SetupBrightWork(PMSIVToolBlendWork *wk, int plane, int start, int end, int wait);
BOOL PMSIVTool_WaitBright(PMSIVToolBlendWork *wk);

#endif // POKEBW2_APP_PMSIV_TOOL_H
