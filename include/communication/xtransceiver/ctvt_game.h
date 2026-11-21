#ifndef POKEBW2_COMMUNICATION_XTRANSCEIVER_CTVT_GAME_H
#define POKEBW2_COMMUNICATION_XTRANSCEIVER_CTVT_GAME_H

#include "types.h"
#include "communication/xtransceiver/ctvt_comm.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The Xtransceiver's minigames: the target game, where the members' faces float up to be touched, and the balloon
// game, where each member pumps their balloon until it pops

// The games, which the host picks
enum {
    CTVT_GAME_TYPE_TARGETS,
    CTVT_GAME_TYPE_BALLOONS,
};

// One of the others in the call, and the pictures of them that go on their faces and balloons
typedef struct {
    BOOL hasCamera;
    BOOL canExchangePhotos;
    u8 netId;
    void *pictures[3];
} CtvtGameMember;

CtvtGame *CtvtGame_Create(CommTvtWork *sys, HeapID heapId);
void CtvtGame_Delete(CommTvtWork *sys, CtvtGame *game);
void CtvtGame_Enter(CommTvtWork *sys, CtvtGame *game);
void CtvtGame_Leave(CommTvtWork *sys, CtvtGame *game);
// Returns the mode to go on to, COMM_TVT_MODE_GAME to stay
int CtvtGame_Main(CommTvtWork *sys, CtvtGame *game);
// CTVT_GAME_TYPE_*
void CtvtGame_SetType(CtvtGame *game, int type);
int CtvtGame_GetType(CtvtGame *game);
G3DManager *CtvtGame_GetG3DManager(CtvtGame *game);
BOOL CtvtGame_IsPlaying(CtvtGame *game);
// What the members answer in the menu to play again, which the game commands carry
void CtvtGame_SetChildQuit(CtvtGame *game, BOOL value);
void CtvtGame_SetHostQuit(CtvtGame *game, BOOL value);
void CtvtGame_SetAllJoined(CtvtGame *game, BOOL value);
void CtvtGame_SetReplayStarted(CtvtGame *game, BOOL value);
void CtvtGame_SetJoined(CtvtGame *game, u8 netId);
void CtvtGame_SetReady(CtvtGame *game, u8 netId);
// Loads the textures of the balloon of netId with the pictures of the member color
void CtvtGame_LoadBalloonPictures(CtvtGame *game, u8 netId, u8 color);
// Launches the target in the free slot index that the host picked. Returns whether it could
BOOL CtvtGame_SpawnTarget(CtvtGame *game, u8 index);
// The seed of the game's random numbers, which the host sends
void CtvtGame_SetSeed(CtvtGame *game, u32 seed);
// The host's frame, which every machine moves the game by
void CtvtGame_SetFrame(CtvtGame *game, u16 frame);
u16 CtvtGame_GetHostFrame(CtvtGame *game);
// The host checks a member's touch against the targets, and sends what it hit
void CtvtGame_CheckTouch(CtvtComm *comm, CtvtGame *game, const CtvtGamePacket *packet, int netId);
// Scores the hits that the host sent, for the members in mask
void CtvtGame_ApplyHits(CtvtGame *game, const CtvtGamePacket *packet, u8 mask);
void CtvtGame_SetScores(CtvtGame *game, const CtvtGameData *data);
// The host counts a member's pump, a big one being worth two, and sends the balloons that grew
void CtvtGame_CountPump(CtvtComm *comm, CtvtGame *game, BOOL big, int netId);
// Grows the balloons of the members in mask
void CtvtGame_PumpBalloons(CtvtGame *game, u8 mask);

// The host's frame that the game is at
u16 CtvtGame_GetFrame(CtvtGame *game);

#endif // POKEBW2_APP_COMM_TVT_CTVT_GAME_H
