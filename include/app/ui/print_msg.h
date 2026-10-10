#ifndef POKEBW2_APP_UI_PRINT_MSG_H
#define POKEBW2_APP_UI_PRINT_MSG_H

#include "types.h"
#include "gfl/str.h"
#include "system/printsys.h"

// print_msg.c, a guessed name: printing a string into a window through the print queue

// Prints a string into a window through the queue, aligned at x by the alignment
void func_ov139_0219a2a4(PrintWindow *window, PrintQueue *queue, u16 x, u16 y, const StrBuf *strbuf, Font *font,
                         u16 color, u32 align);

#endif // POKEBW2_APP_UI_PRINT_MSG_H
