#ifndef POKEBW2_COMMUNICATION_XTRANSCEIVER_CTVT_TALK_H
#define POKEBW2_COMMUNICATION_XTRANSCEIVER_CTVT_TALK_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The Xtransceiver's talk mode: the video chat itself, with push-to-talk voice and the menu buttons

CtvtTalk *CtvtTalk_Create(CommTvtWork *sys, HeapID heapId);
void CtvtTalk_Delete(CommTvtWork *sys, CtvtTalk *talk);
void CtvtTalk_Enter(CommTvtWork *sys, CtvtTalk *talk);
void CtvtTalk_Leave(CommTvtWork *sys, CtvtTalk *talk);
// Returns the mode to go on to, COMM_TVT_MODE_TALK to stay
int CtvtTalk_Main(CommTvtWork *sys, CtvtTalk *talk);
void CtvtTalk_Draw(CommTvtWork *sys, CtvtTalk *talk);
CtvtMic *CtvtTalk_GetMic(CommTvtWork *sys, CtvtTalk *talk);
// What the other members send about a minigame
void CtvtTalk_SetGameInvited(CtvtTalk *talk, BOOL invited);
void CtvtTalk_SetGameCancelRequested(CtvtTalk *talk, BOOL value);
void CtvtTalk_SetGameCancelled(CtvtTalk *talk, BOOL value);
BOOL CtvtTalk_IsGameCancelled(CtvtTalk *talk);
void CtvtTalk_SetGameStarting(CtvtTalk *talk, BOOL value);
void CtvtTalk_SetGameStart(CtvtTalk *talk, BOOL value);
void CtvtTalk_SetJoined(CtvtTalk *talk, u8 member);
BOOL CtvtTalk_AreAllJoined(CommTvtWork *sys, CtvtTalk *talk);
void CtvtTalk_SetReady(CtvtTalk *talk, u8 member);
BOOL CtvtTalk_AreAllReady(CommTvtWork *sys, CtvtTalk *talk);
void CtvtTalk_SetNotAlone(CtvtTalk *talk, BOOL value);
BOOL CtvtTalk_IsNotAlone(CtvtTalk *talk);
void CtvtTalk_SetNotAloneChanged(CtvtTalk *talk, BOOL value);
void CtvtTalk_AddNewMember(CtvtTalk *talk);
void CtvtTalk_ClearNewMembers(CtvtTalk *talk);

#endif // POKEBW2_APP_COMM_TVT_CTVT_TALK_H
