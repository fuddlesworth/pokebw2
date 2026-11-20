#ifndef POKEBW2_FIELD_MEDAL_H
#define POKEBW2_FIELD_MEDAL_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// A medal's entry in archive 0xeb
typedef struct {
    // The same first three fields as MedalDef (scrcmd_pokemon_center.c)
    u32 threshold;
    int check;
    u8 unk8;
    // An index into MEDAL_TYPE_TO_CATEGORY, and the medal's field effect after 0x4e
    u8 type;
    // Whether Mr. Medal hints at the medal while it is unknown
    u8 hintable;
    // The rank the medal counts toward, as MedalDef.rank names it
    u8 rank;
} MedalData;

void DiscoverInitialMedalsCore(MedalBox *box, HeapID heapId);
void DiscoverInitialMedals(MedalBox *box, HeapID heapId);
FieldActor *GetMrMedalActorIndex(Field *field);
u32 GetMrMedalActorUID(GameData *gameData);
u32 CountMedalsByStatus(FieldScriptEnv *env, u32 status);
u32 GetHintableMedalCount(FieldScriptEnv *env);

// The medals' script commands
BOOL Scrcmd_Medal_MedalGetCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Medal_MedalAcknowledge(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Medal_MedalGetSpinTradeActor(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Medal_MedalGive(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Medal_MedalDiscover(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Medal_MedalGetFieldEffectID(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Medal_MedalIsObtained(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Medal_MedalGetMostCompleteCategory(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Medal_MedalDiscoverInitial(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_MEDAL_H
