#ifndef POKEBW2_APP_GSYNC_GSYNC_MESSAGE_H
#define POKEBW2_APP_GSYNC_GSYNC_MESSAGE_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Game Sync's messages (gsync_message.c): the message window and an info window below it on the touch screen,
// and the yes/no menu

// Where GSyncMessage_CreateYesNo puts the menu
#define GSYNC_YESNO_POS_UPPER 0
#define GSYNC_YESNO_POS_LOWER 1

GSyncMessage *GSyncMessage_Create(HeapID heapId, u32 msgFile);
void GSyncMessage_Main(GSyncMessage *msg);
void GSyncMessage_Free(GSyncMessage *msg);
// Prints the loaded string, at once or a character at a time
void GSyncMessage_PrintLoaded(GSyncMessage *msg, BOOL now);
// Prints a message: GSyncMessage_PrintStream a character at a time, GSyncMessage_Print at once
void GSyncMessage_PrintStream(GSyncMessage *msg, u32 msgId);
void GSyncMessage_Print(GSyncMessage *msg, u32 msgId);
// Loads a message with a Pokémon's nickname as word 0 and a number as word 1, to print
void GSyncMessage_FormatPokemonNumber(GSyncMessage *msg, u32 msgId, s32 number, PartyPkm *pkm);
BOOL GSyncMessage_IsPrintFinished(GSyncMessage *msg);
void GSyncMessage_ClearMessage(GSyncMessage *msg);
AppTaskMenu *GSyncMessage_CreateYesNo(GSyncMessage *msg, int pos);
void GSyncMessage_LoadString(GSyncMessage *msg, u32 msgId);
// The info window, with the loaded string, at a row and height of its own or the default ones
void GSyncMessage_PrintInfoAt(GSyncMessage *msg, int y, int height);
void GSyncMessage_PrintInfo(GSyncMessage *msg);
void GSyncMessage_ClearInfo(GSyncMessage *msg);
void GSyncMessage_StartWaitIcon(GSyncMessage *msg);
// Writes a Game Sync ID as its 10 characters
void GSyncMessage_FormatGSyncId(GSyncMessage *msg, u32 id);

#endif // POKEBW2_APP_GSYNC_GSYNC_MESSAGE_H
