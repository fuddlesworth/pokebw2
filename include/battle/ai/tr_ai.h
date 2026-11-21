#ifndef POKEBW2_BATTLE_AI_TR_AI_H
#define POKEBW2_BATTLE_AI_TR_AI_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "struct_decls.h"

// The trainer AI, which scores each of the Pokemon's moves by running an AI script for every flag in its AI flags,
// and chooses the move with the highest score
#define OVERLAY_TR_AI OVERLAY_ID(170)

// The move that TrAI_GetChosenMove returns when the AI decides to flee
#define TRAI_MOVE_FLEE 4

VM *TrAI_CreateVM(BtlMainModule *mainModule, BtlServerFlow *serverFlow, BtlPokeCon *pokeCon, u32 aiFlags,
                  HeapID heapId);
void TrAI_DeleteVM(VM *vm);
// usableMoves has one entry per move slot, 0 if the move can't be chosen
void TrAI_Setup(VM *vm, const u8 *usableMoves, u8 attackerPos);
// Returns TRUE if the AI ran out of time, and has to continue in the next call
BOOL TrAI_Think(VM *vm);
u8 TrAI_GetChosenMove(VM *vm);
u8 TrAI_GetChosenTarget(VM *vm);

#endif // POKEBW2_BATTLE_TR_AI_H
