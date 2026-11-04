#ifndef POKEBW2_FIELD_COLOSSEUM_H
#define POKEBW2_FIELD_COLOSSEUM_H

#include "types.h"
#include "battle/battle_proc.h"
#include "field/comm_player.h"
#include "field/event_colosseum_battle.h"
#include "field/trcard_sys.h"
#include "save/player_info.h"
#include "struct_decls.h"

// colosseum.c, overlay 28, which the ROM names: the Union Room's colosseum, where up to four players gather their
// parties for a battle. Its work is shared with union_main.c and overlay 34

#define COLOSSEUM_MEMBER_MAX 4

// What a player sends the others (0x14 bytes, command 0x1503)
typedef struct {
    u32 unk00[5];
} ColosseumChoice;

// A player of the colosseum
typedef struct {
    PlayerInfo info;
    u8 mac[6];
    u8 present; // TRUE once the player's info is here
    // Whether the parent's entry menu takes the player in without asking: FALSE for one who joined a recruit
    u8 autoAccept;
    u8 unk28;
} ColosseumMember;

// How the parent's entry menu ended, which it sends the others
enum {
    COLOSSEUM_ENTRY_PENDING,
    COLOSSEUM_ENTRY_DONE,
    COLOSSEUM_ENTRY_CANCELLED,
};

struct UnionColosseum {
    CommPlayerSys *commPlayer; // from CommPlayer_Init
    // The player's own position, from CommPlayer_GetMyStatus, which union_comm.c sends when it changed
    CommPlayerStatus myStatus;
    ColosseumMember members[COLOSSEUM_MEMBER_MAX];
    u8 started : 1; // the players are on the colosseum's field, which shows and updates them
    // Set once the parent restarted the link for a net error, so that it does it only once
    u8 relinkRequested : 1;
    u8 unkC8_2 : 6;
    u8 exiting; // set when the player leaves: the colosseum is freed once the field closes
    u8 unkCA;
    u8 allReady; // the parent's COLOSSEUM_CMD_ALL_READY arrived
    // Each player's vote for an entry spot (UnionEff_GetEntrySpot's index), which the parent holds, 0xff for none
    u8 vote[4];
    // The parent's answers to each player's vote, 0xff once sent
    u8 voteReply[4];
    // The players whose ready the parent received, and those who took it back, whom it still has to answer
    u8 readyReceived[4];
    u8 cancelReceived[4];
    // The player's own vote and the parent's reply to it, 0xff for none
    u8 myVote;
    u8 myVoteReply;
    u8 entryResult; // COLOSSEUM_ENTRY_*
    u8 readySent;   // set once the player sent its ready, cleared when the parent answers its cancel
    ColosseumChoice choice[COLOSSEUM_MEMBER_MAX];
    u8 choiceReady[4];
    TrainerCardData *card[COLOSSEUM_MEMBER_MAX];      // where a player's card arrives
    TrainerCardData *cardCheck[COLOSSEUM_MEMBER_MAX]; // the same cards, which the receive checks
    u8 cardReady[4];
    PokeParty *party[COLOSSEUM_MEMBER_MAX];
    u8 partyReady[4];
    // Each player's entry spot, which the parent sends from the votes; bit 0 of a spot is its team
    u8 positions[4];
    u8 positionsReceived;
    u8 unk171;
    u8 leaving[4]; // the players who sent COLOSSEUM_CMD_LEAVE
    u8 unk176[2];
    CommEntryMenu *entryMenu; // the menu that gathers the players for a multi battle
    u8 talkTarget;            // the player spoken to, by net ID
    u8 partySource;           // 0 for the party, 1 for the Battle Box
    u8 entryMask;             // the players the entry menu took, a bit per net ID
    // Set while myStatus waits to be sent
    u8 statusChanged;
};

// Registers the colosseum's net commands, with the UnionSystem as their work
void Colosseum_RegisterCommands(UnionSystem *sys);
// Sends the player's info (to the parent only when toParent is TRUE, else to all), the card, and the card's first part
BOOL Colosseum_SendMember(const ColosseumMember *member, BOOL toParent);
BOOL Colosseum_SendCard(const TrainerCardData *card);
BOOL Colosseum_SendCardHead(const TrainerCardData *card);
BOOL Colosseum_SendChoice(const ColosseumChoice *choice);
BOOL Colosseum_SendVote(UnionColosseum *colosseum);
BOOL Colosseum_SendParty(const PokeParty *party);
BOOL Colosseum_SendPositions(const void *result);
BOOL Colosseum_SendReady(void);
BOOL Colosseum_SendReadyCancel(void);
BOOL Colosseum_SendLeave(void);

// The colosseum's work, made for the player and freed
UnionColosseum *Colosseum_Create(GameData *gameData, GameSystem *gsys, const PlayerInfo *info, u32 joinedRecruit);
void Colosseum_Free(UnionColosseum *colosseum, GameSystem *gsys);
void Colosseum_SetStarted(UnionColosseum *colosseum, BOOL value);
void Colosseum_SetMyVote(UnionColosseum *colosseum, u32 value);
u8 Colosseum_GetMyVoteReply(const UnionColosseum *colosseum);
void Colosseum_SendVoteReplies(UnionColosseum *colosseum);
void Colosseum_UpdateReadySync(UnionColosseum *colosseum);
u8 Colosseum_TakeChoice(UnionColosseum *colosseum, u32 netId, ColosseumChoice *choice);
void Colosseum_ResetUnk171(UnionColosseum *colosseum);
BOOL Colosseum_AreAllPartiesReceived(const UnionColosseum *colosseum);
void Colosseum_ClearParties(UnionColosseum *colosseum, BOOL keepOwn);
void Colosseum_BuildBattlePlayers(UnionColosseum *colosseum, GameSystem *gsys, ColosseumPlayers *players, u32 unused);
void Colosseum_ClearPlayers(ColosseumPlayers *players);

// What colosseum_comm.c and colosseum.c use of each other
BOOL Colosseum_SendVoteReply(UnionColosseum *colosseum, u32 netId, u32 value);
BOOL Colosseum_SendCancelAck(u32 netId);
BOOL Colosseum_SendAllReady(void);
u8 Colosseum_GetMyVote(const UnionColosseum *colosseum);
void Colosseum_SetMyVoteReply(UnionColosseum *colosseum, u32 value);
BOOL Colosseum_Vote(UnionColosseum *colosseum, u32 netId, u8 value);
void Colosseum_StoreChoice(UnionColosseum *colosseum, u32 netId, const ColosseumChoice *choice);

#endif // POKEBW2_FIELD_COLOSSEUM_H
