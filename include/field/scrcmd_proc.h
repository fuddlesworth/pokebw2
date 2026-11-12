#ifndef POKEBW2_FIELD_SCRCMD_PROC_H
#define POKEBW2_FIELD_SCRCMD_PROC_H

// Overlay 12's scrcmd_proc.c: the script commands that leave the field for another screen, such as the bag, the mailbox
// or the Pokédex diplomas, and come back. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

// What Scrcmd_Proc_CallPokedexDiploma opens: the regional and the national Pokédex awards, the Battle Subway's map and
// the Marine Tube's board
enum {
    POKEDEX_DIPLOMA_REGIONAL = 0,
    POKEDEX_DIPLOMA_NATIONAL = 1,
    POKEDEX_DIPLOMA_SUBWAY_MAP = 2,
    POKEDEX_DIPLOMA_MARINE_TUBE_BOARD = 3,
};

// The battle types Scrcmd_Proc_NetConnectWiFiBattle's script argument takes, 15 to 19, which the match numbers 0 to
// 4: single, double, triple, rotation and another triple
enum {
    WIFI_BATTLE_TYPE_SINGLE = 15,
    WIFI_BATTLE_TYPE_DOUBLE = 16,
    WIFI_BATTLE_TYPE_TRIPLE = 17,
    WIFI_BATTLE_TYPE_ROTATION = 18,
    WIFI_BATTLE_TYPE_TRIPLE_2 = 19,
};

// What Scrcmd_Proc_NetConnectBattleVideo opens: the Global Link's Battle Videos or its musical photos
enum {
    BATTLE_VIDEO_MODE_GLOBAL_BV = 0,
    BATTLE_VIDEO_MODE_GLOBAL_MUSICAL = 1,
};

BOOL Scrcmd_Proc_FieldOpen(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_FieldClose(VM *vm, FieldScriptEnv *env);
void CreateScrCmdOverlayProcess(VM *vm, FieldScriptEnv *env, s32 overlayId, const GameProcFunctions *functions,
                                void *resource, void (*cleanup)(ScriptOverlayWork *), BagScriptResult *data);
BOOL WaitScrCmdOverlayProcess(VM *vm, void *data);
BOOL Scrcmd_Proc_RTFreeUserHeap(VM *vm, FieldScriptEnv *env);
void CallBag_ReadResult(ScriptOverlayWork *work);
BOOL Scrcmd_Proc_CallBag(VM *vm, FieldScriptEnv *env);
// Called before the Pokédex diplomas
void CallMailbox_ReadResult(void *arg);
BOOL Scrcmd_Proc_CallMailbox(VM *vm, FieldScriptEnv *env);
void MoveReminder_ReadResult(void *arg);
BOOL Scrcmd_Proc_MoveReminderCallMoveSelect(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_CallMonolith(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_Call3DDemo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_CallPokedexDiploma(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_callPoke3Select(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_Nop(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_NetConnectWiFiBattle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_NetConnectBattleVideo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_GimmickSetNimbasaGym(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_GimmickResetOpelucid(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Proc_GimmickSetNimbasaFlags(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_PROC_H
