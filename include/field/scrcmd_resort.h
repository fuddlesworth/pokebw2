#ifndef POKEBW2_FIELD_SCRCMD_RESORT_H
#define POKEBW2_FIELD_SCRCMD_RESORT_H

#include "types.h"
#include "struct_decls.h"

// The commands of the Join Avenue's script plugin (plugin 8), which RESORT_SCRIPT_COMMANDS lists. Overlay 59's
// scrcmd_resort.c and scrcmd_medalinfo.c have them, and while the shops are open, overlay 60 takes overlay 59's place
// and runs its own command from the first entry's address

// The established queries of Scrcmd_Resort_GetValue; gaps keep their original script values.
enum {
    RESORT_VALUE_ACTOR_UID = 0,
    RESORT_VALUE_CAN_CHANGE_SHOP = 2,
    RESORT_VALUE_RAFFLE_SHOP_ITEM = 3,
    RESORT_VALUE_NO_OCCUPANTS = 6,
    RESORT_VALUE_PERSON_SHOP_KIND = 8,
    RESORT_VALUE_SHOP_FLAG = 9,
    RESORT_VALUE_ALL_OCCUPANTS = 10,
    RESORT_VALUE_CURRENT_SHOP_KIND = 11,
    RESORT_VALUE_HAS_MEDAL = 13,
    RESORT_VALUE_RECENT_ACTIVITY_COUNT = 15,
    RESORT_VALUE_RANK_COMPARISON = 16,
    RESORT_VALUE_SHOP_KIND_0_RECORD_GENDER = 35,
    RESORT_VALUE_SHOP_KIND_2_RECORD_GENDER = 36,
    RESORT_VALUE_SHOP_KIND_0_RECORD_INDEX = 37,
    RESORT_VALUE_SHOP_KIND_2_RECORD_INDEX = 38,
    RESORT_VALUE_DATA_INDEX = 40,
    RESORT_VALUE_PERSON_TYPE = 41,
    RESORT_VALUE_IS_ENTRY_SHOP_KIND_5 = 60,
    RESORT_VALUE_OCCUPANT_COUNT = 64,
    RESORT_VALUE_HAS_SHOP_KIND_0 = 17,
    RESORT_VALUE_OCCUPANT_0_PRESENT = 25,
    RESORT_VALUE_OCCUPANT_0_ACTOR_UID = 44,
    RESORT_VALUE_HAS_SHOP_KIND_1 = 18,
    RESORT_VALUE_OCCUPANT_1_PRESENT = 26,
    RESORT_VALUE_OCCUPANT_1_ACTOR_UID = 45,
    RESORT_VALUE_HAS_SHOP_KIND_3 = 19,
    RESORT_VALUE_OCCUPANT_2_PRESENT = 27,
    RESORT_VALUE_OCCUPANT_2_ACTOR_UID = 46,
    RESORT_VALUE_HAS_SHOP_KIND_2 = 20,
    RESORT_VALUE_OCCUPANT_3_PRESENT = 28,
    RESORT_VALUE_OCCUPANT_3_ACTOR_UID = 47,
    RESORT_VALUE_HAS_SHOP_KIND_4 = 21,
    RESORT_VALUE_OCCUPANT_4_PRESENT = 29,
    RESORT_VALUE_OCCUPANT_4_ACTOR_UID = 48,
    RESORT_VALUE_HAS_SHOP_KIND_5 = 22,
    RESORT_VALUE_OCCUPANT_5_PRESENT = 30,
    RESORT_VALUE_OCCUPANT_5_ACTOR_UID = 49,
    RESORT_VALUE_HAS_SHOP_KIND_6 = 23,
    RESORT_VALUE_OCCUPANT_6_PRESENT = 31,
    RESORT_VALUE_OCCUPANT_6_ACTOR_UID = 50,
    RESORT_VALUE_HAS_SHOP_KIND_7 = 24,
    RESORT_VALUE_OCCUPANT_7_PRESENT = 32,
    RESORT_VALUE_OCCUPANT_7_ACTOR_UID = 51,
};

// How Scrcmd_Resort_MakeData converts the current person.
enum {
    RESORT_MAKE_DATA_OCCUPANT = 0,
    RESORT_MAKE_DATA_RECORD = 1,
    RESORT_MAKE_DATA_OCCUPANT_MOVE_PLAYER = 2,
};

// Visibility actions of Scrcmd_Resort_HideActors.
enum {
    RESORT_HIDE_ACTORS_EXCEPT_SCRIPT_PERSON = 0,
    RESORT_HIDE_ACTORS_ALL_OVERLAPPING = 1,
    RESORT_HIDE_ACTORS_RESTORE = 2,
    RESORT_HIDE_ACTORS_PEOPLE = 3,
    RESORT_HIDE_ACTORS_CAMERA_FILTERED = 4,
};

// The visit operations selected by Scrcmd_Resort_Visit.
enum {
    RESORT_VISIT_CHECK = 0,
    RESORT_VISIT_SCALE_POINTS = 1,
    RESORT_VISIT_ROLL_ACCEPTANCE = 2,
    RESORT_VISIT_GET_MESSAGE = 3,
    RESORT_VISIT_CHOOSE_GROUP = 4,
    RESORT_VISIT_MARK_MET = 5,
};

// The established avenue/player selectors. Selectors 0 to 15 index the saved player values.
enum {
    RESORT_OWN_SHOP_ID = 16,
    RESORT_OWN_SEASON = 17,
    RESORT_OWN_UNOPENED = 18,
    RESORT_OWN_MAX_LEVEL_WEEK = 19,
    RESORT_OWN_UNLOCK = 20,
    RESORT_OWN_CONFIRM_UNLOCK = 21,
    RESORT_OWN_ENTER_EVENT_MODE = 23,
    RESORT_OWN_BUBBLES_ACTIVE = 24,
    RESORT_OWN_SHOP_KIND_5_UNLOCKED = 25,
    RESORT_OWN_PARENT_ACTOR = 26,
    RESORT_OWN_IS_FULL = 27,
    RESORT_OWN_ALL_SHOPS_MAX_LEVEL = 28,
};

// NPC text queries: TEXT_0 begins at column 7; the second group adds 59 to the message ID.
enum {
    RESORT_NPC_VALUE_ACTOR_UID = 0,
    RESORT_NPC_VALUE_ACTOR_INDEX = 1,
    RESORT_NPC_VALUE_TEXT_COLUMN_5 = 2,
    RESORT_NPC_VALUE_TEXT_COLUMN_6 = 3,
    RESORT_NPC_VALUE_TEXT_ROW = 33,
    RESORT_NPC_VALUE_TEXT_0 = 4,
    RESORT_NPC_VALUE_TEXT_0_OFFSET_59 = 16,
    RESORT_NPC_VALUE_TEXT_1 = 5,
    RESORT_NPC_VALUE_TEXT_1_OFFSET_59 = 17,
    RESORT_NPC_VALUE_TEXT_2 = 6,
    RESORT_NPC_VALUE_TEXT_2_OFFSET_59 = 18,
    RESORT_NPC_VALUE_TEXT_3 = 7,
    RESORT_NPC_VALUE_TEXT_3_OFFSET_59 = 19,
    RESORT_NPC_VALUE_TEXT_4 = 8,
    RESORT_NPC_VALUE_TEXT_4_OFFSET_59 = 20,
    RESORT_NPC_VALUE_TEXT_5 = 9,
    RESORT_NPC_VALUE_TEXT_5_OFFSET_59 = 21,
    RESORT_NPC_VALUE_TEXT_6 = 10,
    RESORT_NPC_VALUE_TEXT_6_OFFSET_59 = 22,
    RESORT_NPC_VALUE_TEXT_7 = 11,
    RESORT_NPC_VALUE_TEXT_7_OFFSET_59 = 23,
    RESORT_NPC_VALUE_TEXT_8 = 12,
    RESORT_NPC_VALUE_TEXT_8_OFFSET_59 = 24,
    RESORT_NPC_VALUE_TEXT_9 = 13,
    RESORT_NPC_VALUE_TEXT_9_OFFSET_59 = 25,
    RESORT_NPC_VALUE_TEXT_10 = 14,
    RESORT_NPC_VALUE_TEXT_10_OFFSET_59 = 26,
    RESORT_NPC_VALUE_TEXT_11 = 15,
    RESORT_NPC_VALUE_TEXT_11_OFFSET_59 = 27,
    RESORT_NPC_VALUE_TEXT_COLUMN_0 = 28,
    RESORT_NPC_VALUE_TEXT_COLUMN_1 = 29,
    RESORT_NPC_VALUE_TEXT_COLUMN_2 = 30,
    RESORT_NPC_VALUE_TEXT_COLUMN_3 = 31,
    RESORT_NPC_VALUE_TEXT_COLUMN_4 = 32,
};

// A removal uses the script's selected person rather than a data range.
#define RESORT_REMOVE_CURRENT_PERSON 255

// After the eight occupant indices, recreates all record people.
enum {
    RESORT_RECREATE_RECORD_PEOPLE = 8,
};

BOOL Scrcmd_Resort_StartTalk(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_EndTalk(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_GetValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_GetScriptValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_MakeData(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SetWord(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_RemovePerson(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SaveShopId(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_RaffleShopItem(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SetFriendship(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_RaiseLevel(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_ChangeEffortValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_ShortenEggSteps(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SetShopFlag(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_PayAndSetShopFlag(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_GetPersonVisit(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_AddShopExperience(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_AddPopularity(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SetOccupantsLocked(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_HideActors(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_Visit(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_CheckRank100Unlock(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_NameEntry(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_MarkRaffled(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_GetNPCValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SetOccupantWords(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SetNPCWord(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_DescribeOccupant(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_GetGoodsValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_SetOwnValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_GetOwnValue(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_StartSceneAnim(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_IsSceneAnimDone(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_EndSceneAnim(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_AddEntryOrRecord(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_ShowRecords(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_AddRecordPerson(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Resort_RecreatePeople(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Medalinfo_ShowMedalInfo(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_RESORT_H
