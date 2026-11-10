#ifndef POKEBW2_DPW_DPW_BT_H
#define POKEBW2_DPW_DPW_BT_H

#include "types.h"
#include "dpw/dpw_tr.h"

// The Battle Subway's server library in overlay 189 ("/web/battletower/*.asp"), which the Wi-Fi Battle Subway of
// overlay 191 talks to. It is built like dpw_tr.h's (a state that the game runs every frame, a result that is 0 or more
// when the call went well). The ROM has no strings of this library's own, so the DpwBt_ names are guesses after the
// server's pages

// The results of a request that did not go well. The pages' answers give CROWDED and INVALID_NUMBER, and
// DpwBt_Cancel gives CANCELED. ERROR is a request that cannot start, an answer the library does not know, or a fatal
// error of the game statistics client, which the screen leaves to the Wi-Fi library's own error screen.
enum {
    // The train's room is full; the screen reports it as "The Wi-Fi Train is very crowded"
    DPW_BT_RESULT_CROWDED = -1,
    // The train's number is not one the server has
    DPW_BT_RESULT_INVALID_NUMBER = -3,
    // DpwBt_Cancel stopped the request
    DPW_BT_RESULT_CANCELED = -4,
    DPW_BT_RESULT_ERROR = -5,
};

// The server's state, which DpwBt_GetServerState's and DpwBt_SetProfile's answers give through the result:
// the train service is in maintenance or crowded
enum {
    DPW_BT_SERVER_STATE_MAINTENANCE = 1,
    DPW_BT_SERVER_STATE_CROWDED = 2,
};

// Starts and ends the library, for the player's ID and friend key, and runs it every frame
void DpwBt_Init(s32 pid, u64 friendKey, int server);
void DpwBt_End(void);
void DpwBt_Process(void);
// Whether the last request has ended, and its result
BOOL DpwBt_IsFinished(void);
s32 DpwBt_GetResult(void);
// Asks for the number of the room that the given kind of battle has
void DpwBt_GetRoomNumber(int kind);
// Asks for the Pokémon of a room: the kind of battle and the room's number; they go to the result
void DpwBt_Download(int kind, int room, void *result);
// Uploads a team of 0xf0 bytes for the kind of battle and the room, with a byte of the result, a signature and a word
void DpwBt_Upload(int kind, int room, u8 win, const void *team, const void *sign, u32 id);
// Cancels the request
void DpwBt_Cancel(void);
// Asks the server's state
void DpwBt_GetServerState(void);
// Sends the player's profile
void DpwBt_SetProfile(Dpw_Common_Profile *profile, Dpw_Common_ProfileResult *result);

#endif // POKEBW2_DPW_DPW_BT_H
