#ifndef POKEBW2_IRC_IRC_LINK_H
#define POKEBW2_IRC_IRC_LINK_H

#include "types.h"

// irc_link.c, in overlay 31: the SDK's libirc irc.c, the link of frames between two machines over infrared (see
// net_devirc.h); lib/irc is this project's name for it

// The frames the link sends between the two machines start with one of these. Any byte under IRC_CMD_MIN is a
// packet of data
// The command set is this project's own: aletheia's IRCCommand names are its reading of the flow (0x56 is
// CONNECT_REQUEST, 0xfc is DISCONNECT), not names recovered from the library's DWARF, so they are not SDK names.
#define IRC_CMD_MIN 0xf0
#define IRC_CMD_EMPTY 0xee
#define IRC_CMD_CLOSE 0xf4
#define IRC_CMD_RESEND 0xf6
#define IRC_CMD_ACCEPT 0xf8
#define IRC_CMD_REPLY 0xfa
#define IRC_CMD_PING 0xfc

void IRC_Init(u8 unitId);
void IRC_Connect(void);
BOOL IRC_IsSender(void);
BOOL IRC_IsConnect(void);
void IRC_Shutdown(void);
void IRC_Send(const u8 *data, u8 size, u8 command);
BOOL IRC_Move(void);
void IRC_SetRecvCallback(void (*callback)(u8 *data, u8 size, u8 command, u8 id));
u8 IRC_GetUnitNumber(void);

#endif // POKEBW2_IRC_IRC_LINK_H
