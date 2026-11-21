#ifndef POKEBW2_WIFI_GSYNC_PDWACC_MESSAGE_H
#define POKEBW2_WIFI_GSYNC_PDWACC_MESSAGE_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The Dream World account screens' messages (pdwacc_message.c): the message window and an info window on the touch
// screen, the yes/no menu, and the Game Sync ID on the top screen. The parameter types are guessed from this file's
// code

// Where PdwAccMessage_CreateYesNo puts the menu
#define PDWACC_YESNO_POS_UPPER 0
#define PDWACC_YESNO_POS_LOWER 1

PdwAccMessage *PdwAccMessage_Create(HeapID heapId, u32 msgFile);
void PdwAccMessage_Main(PdwAccMessage *msg);
void PdwAccMessage_Free(PdwAccMessage *msg);
// Prints a message a character at a time
void PdwAccMessage_PrintStream(PdwAccMessage *msg, u32 msgId);
void PdwAccMessage_StartWaitIcon(PdwAccMessage *msg);
BOOL PdwAccMessage_IsPrintFinished(PdwAccMessage *msg);
void PdwAccMessage_ClearMessage(PdwAccMessage *msg);
AppTaskMenu *PdwAccMessage_CreateYesNo(PdwAccMessage *msg, int pos);
void PdwAccMessage_PrintInfo(PdwAccMessage *msg, u32 msgId);
void PdwAccMessage_ClearInfo(PdwAccMessage *msg);
// Shows a Game Sync ID as its 10 characters, under its label. Both callers pass the ID again as the third argument,
// which is unused
void PdwAccMessage_ShowGSyncId(PdwAccMessage *msg, u32 id, u32 unused);
void PdwAccMessage_ClearGSyncId(PdwAccMessage *msg);

#endif // POKEBW2_APP_GSYNC_PDWACC_MESSAGE_H
