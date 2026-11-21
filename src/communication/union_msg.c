#include "field/union_msg.h"
#include "types.h"
#include "constants/language.h"
#include "constants/text_banks.h"
#include "field/field.h"
#include "field/field_msgbg.h"
#include "field/union_oneself.h"
#include "field/unity_tower.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "system/bmp_menulist.h"
#include "system/game_data.h"
#include "system/pms_word.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/union_room_messages.h"

// The Union Room's messages and menus: the lookups of message IDs, the message system the field talks through (the
// message data, the word set, the stream window), the menus of the battle rules and the regulation's rules printed.
// Names are not swan's

#define UNION_HEAPID 0x41

typedef struct {
    const ListMenuRow *rows;
    u8 count;
} UnionMenuInfo;

static void UnionMsg_SetupWordSet(UnionSystem *unisys);
static void UnionMsg_SetLanguageFlag(GameData *gameData, u32 language);
static void UnionMsg_CreateWordSet(UnionSystem *unisys);
static void UnionMsg_FreeWordSet(UnionSystem *unisys);
static void UnionMsg_FreeMsgSystem(UnionSystem *unisys);
static void UnionMsg_CreateMenu(UnionSystem *unisys, Field *field, const ListMenuRow *rows, u32 count,
                                ListMenuRequest *request);
static void UnionMsg_FreeMenu(UnionSystem *unisys);
static s32 UnionMsg_UpdateMenu(UnionSystem *unisys, Field *field);

// The rows of the summaries that end the menu: confirm the rules, or show them
#define UNION_RULE_MENU_CONFIRM 0x64
#define UNION_RULE_MENU_SHOW_RULES 0x96

// The menus of the battle rules, each row a message and a value or the menu it opens
static const ListMenuRow sMenuRules[3];
static const ListMenuRow sMenuLauncher[3];
static const ListMenuRow sMenuPlayers[3];
static const ListMenuRow sMenuFourRules[3];
static const ListMenuRow sMenuFourLauncher[3];
static const ListMenuRow sMenuFormat[5];
static const ListMenuRow sMenuFour[5];
static const ListMenuRow sMenuTwo[6];

// The tables are declared in the order that gives the ROM's .rodata: MWCC heapsorts the objects of equal size by where
// they are declared. The pairs are the messages of a gender, or of a language, by the lookups above
static const u16 sMsgPair_0214[2] = { UnionRoomMessages_Text_ThanksWaitingLetsGet,
                                      UnionRoomMessages_Text_SorryMadeWaitLets };

static const u16 sMsgPair_0210[2] = { UnionRoomMessages_Text_ThereEggCannotTraded_3,
                                      UnionRoomMessages_Text_ThereEggCannotTraded_4 };

static const u16 sMsgPair_0208[2] = { UnionRoomMessages_Text_CantTradeIfHave,
                                      UnionRoomMessages_Text_CantTradeIfHave_2 };

static const u16 sMsgPair_01f0[2] = { UnionRoomMessages_Text_OhAllRightCome_3,
                                      UnionRoomMessages_Text_OhPleaseComeBy_3 };

static const u16 sMsgPair_0218[2] = { UnionRoomMessages_Text_IfWantSomethingJust,
                                      UnionRoomMessages_Text_IfWantSomethingDont };

static const u16 sMsgPair_0220[2] = { UnionRoomMessages_Text_ThereEggCannotTraded,
                                      UnionRoomMessages_Text_ThereEggCannotTraded_2 };

static const u16 sMsgPair_021c[2] = { UnionRoomMessages_Text_OkJustGiveSecond_3,
                                      UnionRoomMessages_Text_OkJustGiveSecond_4 };

static const u16 sMsgPair_0228[2] = { UnionRoomMessages_Text_WereDrawingRightNow,
                                      UnionRoomMessages_Text_WereDrawingRightNow_2 };

static const u16 sMsgPair_01f4[2] = { UnionRoomMessages_Text_SorryMakeWaitOk,
                                      UnionRoomMessages_Text_SorryKeepWaitingOk };

static const u16 sMsgPair_01e0[2] = { UnionRoomMessages_Text_OkJustGiveSecond,
                                      UnionRoomMessages_Text_OkJustGiveSecond_2 };

static const u16 sMsgPair_0224[2] = { UnionRoomMessages_Text_TrainerAppearsBusy,
                                      UnionRoomMessages_Text_TrainerAppearsBusy_2 };

static const u16 sMsgPair_0204[2] = { UnionRoomMessages_Text_DidNotReply, UnionRoomMessages_Text_DidNotReply_2 };

static const u16 sMsgPair_01f8[2] = { UnionRoomMessages_Text_WereGoingSpinTrade,
                                      UnionRoomMessages_Text_WeWereDoingSpin };

static const u16 sMsgPair_0230[2] = { UnionRoomMessages_Text_WhoopsSorryButHave,
                                      UnionRoomMessages_Text_WhoopsSorryButHave_2 };

static const u16 sMsgPair_020c[2] = { UnionRoomMessages_Text_ThereEggCannotTraded,
                                      UnionRoomMessages_Text_ThereEggCannotTraded_2 };

static const u16 sMsgPair_01ec[2] = { UnionRoomMessages_Text_OhAllRightCome_2,
                                      UnionRoomMessages_Text_OhPleaseComeBy_2 };

static const u16 sMsgPair_0200[2] = { UnionRoomMessages_Text_NeedEggIfWant, UnionRoomMessages_Text_NeedEggIfWant_2 };

static const u16 sMsgPair_01e8[2] = { UnionRoomMessages_Text_WhatWouldLike, UnionRoomMessages_Text_WhatWouldLike_2 };

static const u16 sMsgPair_022c[2] = { UnionRoomMessages_Text_OhAllRightCome, UnionRoomMessages_Text_OhPleaseComeBy };

static const u16 sMsgPair_01fc[2] = { UnionRoomMessages_Text_WereAboutHaveMulti,
                                      UnionRoomMessages_Text_WereAboutHaveMulti_2 };

static const u16 sMsgPair_01e4[2] = { UnionRoomMessages_Text_OhImSorryHave_3,
                                      UnionRoomMessages_Text_OhSorryLetsAnother };

static const u16 sMsgPair_01d8[2] = { UnionRoomMessages_Text_OkLetsSomeDrawing,
                                      UnionRoomMessages_Text_OkThanksWaitingLets };

static const u16 sMsgPair_01dc[2] = { UnionRoomMessages_Text_OhImSorryHave, UnionRoomMessages_Text_OhImSorryHave_2 };

static const u16 sMsgPair_0244[2] = { UnionRoomMessages_Text_OkJustGiveSecond_5,
                                      UnionRoomMessages_Text_OkJustGiveSecond_6 };

static const u16 sMsgPair_0240[2] = { UnionRoomMessages_Text_OkLetsSpinTrade,
                                      UnionRoomMessages_Text_OkLetsSpinTrade_2 };

static const u16 sMsgPair_023c[2] = { UnionRoomMessages_Text_OhImSorryHave_5,
                                      UnionRoomMessages_Text_OhSorryLetsAnother_3 };

static const u16 sMsgPair_0238[2] = { UnionRoomMessages_Text_WhatsLooksLikeDont,
                                      UnionRoomMessages_Text_WhatsLooksLikeDont_2 };

static const u16 sMsgPair_0234[2] = { UnionRoomMessages_Text_ThereSomethingWanted,
                                      UnionRoomMessages_Text_HelloItsWouldLike };

static const u16 sMsgPairs_0248[2][2] = {
    { UnionRoomMessages_Text_OhYeahReallyWanted, UnionRoomMessages_Text_ImTradingPokemonRight },
    { UnionRoomMessages_Text_FinallyGotPokemonIve, UnionRoomMessages_Text_FinallyGotPokemonWanted },
};

static const u16 sMsgPairs_0250[2][2] = {
    { UnionRoomMessages_Text_ImJustShowingTrainer, UnionRoomMessages_Text_WereShowingEachOther },
    { UnionRoomMessages_Text_HopeGetKnowBetter, UnionRoomMessages_Text_GladMeetPleaseDont },
};

static const u16 sMsgPairs_0258[4][2] = {
    { UnionRoomMessages_Text_LastLetterWhatDraw, UnionRoomMessages_Text_WowYoureReallyGood },
    { UnionRoomMessages_Text_StopDrawingFunnyFaces, UnionRoomMessages_Text_AhahaWhat },
    { UnionRoomMessages_Text_WowYoureGoodIll, UnionRoomMessages_Text_LoveDrawingCanDraw },
    { UnionRoomMessages_Text_UhHuhWhatSupposed, UnionRoomMessages_Text_OopsCantStayInside },
};

static const u16 sMsgPairs_0268[4][2] = {
    { UnionRoomMessages_Text_WhoaCanTellYoure, UnionRoomMessages_Text_PokemonItsBeenRaised },
    { UnionRoomMessages_Text_UsedMoveThatsGood, UnionRoomMessages_Text_ThatsRightMoveNow },
    { UnionRoomMessages_Text_WayGoEyeOpener, UnionRoomMessages_Text_ThatsAwesomeIveNever },
    { UnionRoomMessages_Text_OhHowCouldUse, UnionRoomMessages_Text_HaveExquisiteTimingSwitching },
};

static const u16 sMsgPairs_0278[4][2] = {
    { UnionRoomMessages_Text_SpinTradesMuchFun, UnionRoomMessages_Text_WereDoingSpinTrades },
    { UnionRoomMessages_Text_ThereStopThere, UnionRoomMessages_Text_WahMovedAgain },
    { UnionRoomMessages_Text_EggsAwesome, UnionRoomMessages_Text_ThereGoesEggWanted },
    { UnionRoomMessages_Text_YesGotEggWanted, UnionRoomMessages_Text_IfStoppedThereWouldve },
};

static const u16 sMsgPairs_04cc[25][2] = {
    { UnionRoomMessages_Text_SureThingLetShow, UnionRoomMessages_Text_HiLetIntroduceMyself },
    { UnionRoomMessages_Text_OhDidWantDraw, UnionRoomMessages_Text_WantDrawSureJust },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_BattleCourseButNeed, UnionRoomMessages_Text_BattleCourseButNeed_2 },
    { UnionRoomMessages_Text_WantTradeHuhSure, UnionRoomMessages_Text_TradeCourseButNeed },
    { UnionRoomMessages_Text_SpinTradeSureOk, UnionRoomMessages_Text_SpinTradeCourseOk },
    { UnionRoomMessages_Text_SpinTradeSureOk, UnionRoomMessages_Text_SpinTradeCourseOk },
};

static const u16 sMsgPairs_0530[25][2] = {
    { UnionRoomMessages_Text_HuhTrainerCardWhered, UnionRoomMessages_Text_HuhTrainerCardWhered_2 },
    { UnionRoomMessages_Text_OhSorryCantNow, UnionRoomMessages_Text_OhImSorryLets },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryPokemonDontSeem, UnionRoomMessages_Text_ImTerriblySorryBut },
    { UnionRoomMessages_Text_SorryDontHavePokemon, UnionRoomMessages_Text_ImSorryButDont },
    { UnionRoomMessages_Text_OhImSorryHave_4, UnionRoomMessages_Text_OhSorryLetsAnother_2 },
    { UnionRoomMessages_Text_Empty_20, UnionRoomMessages_Text_Empty_21 },
};

static const u16 sTable_0424[26] = { 11, 18, 22, 22, 23, 23, 25, 25, 26, 26, 28, 28, 29,
                                     29, 31, 31, 32, 32, 33, 33, 34, 34, 37, 44, 41, 0 };

static const u16 sTable_0458[26] = { 17, 20, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36,
                                     36, 36, 36, 36, 36, 36, 36, 36, 36, 39, 46, 46, 0 };

static const ListMenuRow sMenuRules[3] = {
    { UnionRoomMessages_Text_NoRestrictions, { 0x0 } },
    { UnionRoomMessages_Text_FlatBattle, { 0x1 } },
    { UnionRoomMessages_Text_Cancel_6, { .sub = sMenuTwo } },
};

static const ListMenuRow sMenuLauncher[3] = {
    { UnionRoomMessages_Text_WonderLauncher, { 0x0 } },
    { UnionRoomMessages_Text_WonderLauncherOff, { 0x1 } },
    { UnionRoomMessages_Text_Cancel_6, { .sub = sMenuTwo } },
};

static const ListMenuRow sMenuPlayers[3] = {
    { UnionRoomMessages_Text_BattlesTwo, { .sub = sMenuTwo } },
    { UnionRoomMessages_Text_BattlesFour, { .sub = sMenuFour } },
    { UnionRoomMessages_Text_Cancel_4, { BMPMENULIST_CANCEL } },
};

static const ListMenuRow sMenuFourRules[3] = {
    { UnionRoomMessages_Text_NoRestrictions, { 0x0 } },
    { UnionRoomMessages_Text_FlatBattle, { 0x1 } },
    { UnionRoomMessages_Text_Cancel_6, { .sub = sMenuFour } },
};

static const ListMenuRow sMenuFourLauncher[3] = {
    { UnionRoomMessages_Text_WonderLauncher, { 0x0 } },
    { UnionRoomMessages_Text_WonderLauncherOff, { 0x1 } },
    { UnionRoomMessages_Text_Cancel_6, { .sub = sMenuFour } },
};

// clang-format off
static const ListMenuRow sMenuFormat[5] = {
    { UnionRoomMessages_Text_SingleBattle, { UNION_BATTLE_FORMAT_SINGLE } },
    { UnionRoomMessages_Text_DoubleBattle, { UNION_BATTLE_FORMAT_DOUBLE } },
    { UnionRoomMessages_Text_TripleBattle, { UNION_BATTLE_FORMAT_TRIPLE } },
    { UnionRoomMessages_Text_RotationBattle, { UNION_BATTLE_FORMAT_ROTATION } },
    { UnionRoomMessages_Text_Cancel_5, { .sub = sMenuTwo } },
};

// clang-format on

static const ListMenuRow sMenuFour[5] = {
    { UnionRoomMessages_Text_DoubleBattle, { .sub = sMenuFourRules } },
    { UnionRoomMessages_Text_TripleBattle, { .sub = sMenuFourLauncher } },
    { UnionRoomMessages_Text_Confirm, { UNION_RULE_MENU_CONFIRM } },
    { UnionRoomMessages_Text_Rules, { UNION_RULE_MENU_SHOW_RULES } },
    { UnionRoomMessages_Text_Cancel_5, { .sub = sMenuPlayers } },
};

static const ListMenuRow sMenuTwo[6] = {
    { UnionRoomMessages_Text_SingleBattle, { .sub = sMenuFormat } },
    { UnionRoomMessages_Text_DoubleBattle, { .sub = sMenuRules } },
    { UnionRoomMessages_Text_TripleBattle, { .sub = sMenuLauncher } },
    { UnionRoomMessages_Text_Confirm, { UNION_RULE_MENU_CONFIRM } },
    { UnionRoomMessages_Text_Rules, { UNION_RULE_MENU_SHOW_RULES } },
    { UnionRoomMessages_Text_Cancel_5, { .sub = sMenuPlayers } },
};

static const UnionMenuInfo sMenuInfo[8] = {
    { sMenuPlayers, 3 },  { sMenuTwo, 6 },  { sMenuFormat, 5 },    { sMenuRules, 3 },
    { sMenuLauncher, 3 }, { sMenuFour, 5 }, { sMenuFourRules, 3 }, { sMenuFourLauncher, 3 },
};

// The party and the rules a menu's choices lead to, by the battle format and the two options
static const u8 sRuleResult[5][2][2] = {
    { { UNION_ACTIVITY_SINGLE, UNION_ACTIVITY_SINGLE_NO_LAUNCHER },
      { UNION_ACTIVITY_SINGLE_FLAT, UNION_ACTIVITY_SINGLE_FLAT_NO_LAUNCHER } },
    { { UNION_ACTIVITY_DOUBLE, UNION_ACTIVITY_DOUBLE_NO_LAUNCHER },
      { UNION_ACTIVITY_DOUBLE_FLAT, UNION_ACTIVITY_DOUBLE_FLAT_NO_LAUNCHER } },
    { { UNION_ACTIVITY_TRIPLE, UNION_ACTIVITY_TRIPLE_NO_LAUNCHER },
      { UNION_ACTIVITY_TRIPLE_FLAT, UNION_ACTIVITY_TRIPLE_FLAT_NO_LAUNCHER } },
    { { UNION_ACTIVITY_ROTATION, UNION_ACTIVITY_ROTATION_NO_LAUNCHER },
      { UNION_ACTIVITY_ROTATION_FLAT, UNION_ACTIVITY_ROTATION_FLAT_NO_LAUNCHER } },
    { { UNION_ACTIVITY_MULTI, UNION_ACTIVITY_MULTI_NO_LAUNCHER },
      { UNION_ACTIVITY_MULTI_FLAT, UNION_ACTIVITY_MULTI_FLAT_NO_LAUNCHER } },
};

static const ListMenuRow sMenuMain[6] = {
    { UnionRoomMessages_Text_Greet, { UNION_ACTIVITY_CARD } },
    { UnionRoomMessages_Text_Battle, { UNION_ACTIVITY_NONE } },
    { UnionRoomMessages_Text_Trade, { UNION_ACTIVITY_TRADE } },
    { UnionRoomMessages_Text_Draw, { UNION_ACTIVITY_DRAW } },
    { UnionRoomMessages_Text_Spin, { UNION_ACTIVITY_SPIN_TRADE } },
    { UnionRoomMessages_Text_Cancel, { 0 } },
};

static const u32 sLanguageFlags[8] = { 0, 1, 2, 3, 4, 7, 5, 6 };

static const ListMenuRequest sMenuMainRequest = { 6, 6, 0, 0x0d, 0,    0, 1, 0xf,  2,    0,
                                                  0, 0, 1, 0x0c, 0x10, 1, 1, 0x0d, 0x0c, NULL };

static const ListMenuRequest sMenuRulesRequest = { 3, 3, 0, 0x0d, 0,    0, 1, 0xf,  2,    0,
                                                   0, 0, 1, 0x0c, 0x10, 1, 1, 0x13, 0x0e, NULL };

static const struct {
    u8 count;
    u8 pad[3];
} sActivityPlayerCounts[27] = {
    { 2 }, { 2 }, { 2 }, { 5 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 },
    { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 2 }, { 4 }, { 4 }, { 4 }, { 4 }, { 2 }, { 5 }, { 5 },
};

u16 UnionMsg_GetNoReplyMessage(UnionSystem *unisys) {
    return sMsgPair_0204[getTrainerGender(unisys->param->playerInfo)];
}

u16 UnionMsg_GetContactGreetingMessage(u8 gender) {
    return sMsgPair_0234[gender];
}

u16 UnionMsg_GetJustAskMessage(u8 gender) {
    return sMsgPair_0218[gender];
}

u16 UnionMsg_GetBusyMessage(u8 gender) {
    return sMsgPair_0230[gender];
}

u16 UnionMsg_GetShowCardMessage(unsigned int row, u8 gender) {
    return sMsgPairs_04cc[row - 2][gender];
}

u16 UnionMsg_GetCardMissingMessage(unsigned int row, u8 gender) {
    return sMsgPairs_0530[row - 2][gender];
}

u16 UnionMsg_GetOfferMessage(unsigned int row) {
    return sTable_0424[row - 2];
}

u16 UnionMsg_GetDeclinedMessage(unsigned int row) {
    return sTable_0458[row - 2];
}

u16 UnionMsg_GetWhatToDoMessage(u8 gender) {
    return sMsgPair_01e8[gender];
}

u16 UnionMsg_GetBattleStartMessage(u8 gender) {
    return sMsgPair_0214[gender];
}

u16 UnionMsg_GetOnePokemonCannotTradeMessage(u8 gender) {
    return sMsgPair_0208[gender];
}

u16 UnionMsg_GetNeedEggMessage(u8 gender) {
    return sMsgPair_0200[gender];
}

u16 UnionMsg_GetEggCannotTradeMessage(UnionSystem *unisys) {
    return sMsgPair_0210[getTrainerGender(unisys->param->playerInfo)];
}

u16 UnionMsg_GetEggCannotTradeResultMessage(UnionSystem *unisys) {
    return sMsgPair_020c[getTrainerGender(unisys->param->playerInfo)];
}

u16 UnionMsg_GetBattleChatMessage(u8 gender) {
    return sMsgPairs_0268[GFL_RandomLC(4)][gender];
}

u16 UnionMsg_GetBusyTrainerMessage(u8 gender) {
    return sMsgPair_0224[gender];
}

u16 UnionMsg_GetCardChatMessage(u8 gender) {
    return sMsgPairs_0250[GFL_RandomLC(2)][gender];
}

u16 UnionMsg_GetDrawChatMessage(u8 gender) {
    return sMsgPairs_0258[GFL_RandomLC(4)][gender];
}

u16 UnionMsg_GetTradeChatMessage(u8 gender) {
    return sMsgPairs_0248[GFL_RandomLC(2)][gender];
}

u16 UnionMsg_GetSpinTradeChatMessage(u8 gender) {
    return sMsgPairs_0278[GFL_RandomLC(4)][gender];
}

u16 UnionMsg_GetJoinMultiBattleMessage(u8 gender) {
    return sMsgPair_01fc[gender];
}

u16 UnionMsg_GetJoinSpinTradeMessage(u8 gender) {
    return sMsgPair_01f8[gender];
}

u16 UnionMsg_GetJoinDrawMessage(u8 gender) {
    return sMsgPair_0228[gender];
}

u16 UnionMsg_GetBattleDeclineMessage(u8 gender) {
    return sMsgPair_01ec[gender];
}

u16 UnionMsg_GetEggTradeDeclineMessage(u8 gender) {
    return sMsgPair_01f0[gender];
}

u16 UnionMsg_GetDrawDeclineMessage(u8 gender) {
    return sMsgPair_022c[gender];
}

u16 UnionMsg_GetNoEggMessage(u8 gender) {
    return sMsgPair_0238[gender];
}

u16 UnionMsg_GetTargetEggCannotTradeMessage(u8 gender) {
    return sMsgPair_0220[gender];
}

u16 UnionMsg_GetEggTradeAcceptMessage(u8 gender) {
    return sMsgPair_0244[gender];
}

u16 UnionMsg_GetSpinTradeStartMessage(u8 gender) {
    return sMsgPair_0240[gender];
}

u16 UnionMsg_GetEggTradeBusyMessage(u8 gender) {
    return sMsgPair_023c[gender];
}

u16 UnionMsg_GetDrawAcceptMessage(u8 gender) {
    return sMsgPair_01e0[gender];
}

u16 UnionMsg_GetDrawStartMessage(u8 gender) {
    return sMsgPair_01d8[gender];
}

u16 UnionMsg_GetDrawBusyMessage(u8 gender) {
    return sMsgPair_01dc[gender];
}

u16 UnionMsg_GetBattleAcceptMessage(u8 gender) {
    return sMsgPair_021c[gender];
}

u16 UnionMsg_GetMultiBattleStartMessage(u8 gender) {
    return sMsgPair_01f4[gender];
}

u16 UnionMsg_GetBattleBusyMessage(u8 gender) {
    return sMsgPair_01e4[gender];
}

u32 UnionMsg_GetOriginMessage(UnionSystem *unisys, u8 country, u8 province) {
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(unisys->param->gameData);
    u8 myCountry = UnityTowerVisitor_GetCountry(playerInfo);
    u8 myProvince = UnityTowerVisitor_GetProvince(playerInfo);

    if (country == 0) {
        return UnionRoomMessages_Text_STrainerCard;
    }
    if (myCountry != country) {
        if (province == 0) {
            return UnionRoomMessages_Text_TrainerCardWhoLives_2;
        }
        return UnionRoomMessages_Text_TrainerCardWhoLives;
    }
    if (province == 0 || myProvince == province) {
        return UnionRoomMessages_Text_STrainerCard;
    }
    return UnionRoomMessages_Text_TrainerCardWhoLives_3;
}

// The words of the message system: the player or the one spoken to, their greeting by language and their origin
static void UnionMsg_SetupWordSet(UnionSystem *unisys) {
    UnionSelf *situ = &unisys->situ;
    PlayerInfo *playerInfo = unisys->param->playerInfo;
    const u16 *name;
    u8 language;
    u8 gender;
    StrBuf *strbuf;
    u32 messageId;
    UnionBeaconEntry *target;

    GFL_ASSERT(unisys->msgdata != NULL);
    GFL_ASSERT(unisys->wordset != NULL);

    target = situ->mycomm.target;
    if (target != NULL) {
        name = target->beacon.name;
        language = target->beacon.language;
        gender = target->beacon.gender;
    } else {
        name = GetPlayerName(playerInfo);
        language = LANGUAGE_ENGLISH;
        gender = getTrainerGender(playerInfo);
    }

    strbuf = GFL_StrBufCreate(0x20, UNION_HEAPID);
    GFL_StrBufLoadString(strbuf, name);
    GFL_WordSetSetStrbuf(unisys->wordset, 0, strbuf, gender, TRUE, language);
    GFL_StrBufFree(strbuf);
    copyVarForText(unisys->wordset, 1, playerInfo);

    switch (language) {
    case LANGUAGE_JAPANESE:
        messageId = UnionRoomMessages_Text_Empty_71;
        break;
    case LANGUAGE_ENGLISH:
        messageId = UnionRoomMessages_Text_Hello;
        break;
    case LANGUAGE_FRENCH:
        messageId = UnionRoomMessages_Text_Bonjour;
        break;
    case LANGUAGE_ITALIAN:
        messageId = UnionRoomMessages_Text_Ciao;
        break;
    case LANGUAGE_GERMAN:
        messageId = UnionRoomMessages_Text_Hallo;
        break;
    case LANGUAGE_SPANISH:
        messageId = UnionRoomMessages_Text_Hola;
        break;
    case LANGUAGE_KOREAN:
        messageId = UnionRoomMessages_Text_Empty_72;
        break;
    default:
        messageId = UnionRoomMessages_Text_HELL;
        break;
    }
    strbuf = GFL_MsgDataLoadStrbufNew(unisys->msgdata, messageId);
    GFL_WordSetSetStrbuf(unisys->wordset, 2, strbuf, gender, TRUE, language);
    GFL_StrBufFree(strbuf);

    if (situ->mycomm.target != NULL && situ->mycomm.target->beacon.country != 0) {
        loadCountryToStrbuf(unisys->wordset, 3, situ->mycomm.target->beacon.country);
        if (situ->mycomm.target->beacon.province != 0) {
            loadCountryAreaToStrbuf(unisys->wordset, 4, situ->mycomm.target->beacon.country,
                                    situ->mycomm.target->beacon.province);
        }
    }
    UnionMsg_SetLanguageFlag(unisys->param->gameData, language);
}

// Marks the language as seen in the player's Pokemon Wonder word list
static void UnionMsg_SetLanguageFlag(GameData *gameData, u32 language) {
    PMSWordSave *save = getDexBlkAddress(GameData_GetSaveControl(gameData));
    u16 index = language - 1;

    if (language >= 1 && language <= 8) {
        if (index < 8) {
            PMSWordSave_SetLanguageFlag(save, sLanguageFlags[index]);
        } else {
            PMSWordSave_SetLanguageFlag(save, 7);
        }
    } else {
        PMSWordSave_SetLanguageFlag(save, 7);
    }
}

static void UnionMsg_CreateWordSet(UnionSystem *unisys) {
    if (unisys->wordset == NULL) {
        unisys->wordset = GFL_WordSetSystemCreate(5, 0x40, UNION_HEAPID);
        UnionMsg_SetupWordSet(unisys);
    }
}

static void UnionMsg_FreeWordSet(UnionSystem *unisys) {
    if (unisys->wordset != NULL) {
        GFL_WordSetSystemFree(unisys->wordset);
        unisys->wordset = NULL;
    }
}

s32 UnionMsg_GetActivityPlayerCount(s32 index) {
    if (index >= 27) {
        return 0;
    }
    return sActivityPlayerCounts[index].count;
}

void UnionMsg_FreeAll(UnionSystem *unisys) {
    UnionMsg_FreeRulesWindow(unisys);
    UnionMsg_FreeInfoWindow(unisys);
    UnionMsg_FreeYesNo(unisys);
    UnionMsg_FreeMenu(unisys);
    UnionMsg_FreeMsgSystem(unisys);
}

void UnionMsg_CreateMsgSystem(UnionSystem *unisys, Field *field) {
    void *fldmsg_bg;

    if (unisys->fld_msgwin_stream == NULL) {
        GFL_ASSERT(unisys->msgdata == NULL);
        fldmsg_bg = Field_GetMsgBGSys(field);
        unisys->msgdata = MsgBGPrinter_CreateMsgData(fldmsg_bg, TEXT_BANK_UNION_ROOM_MESSAGES);
        unisys->msgbuf = GFL_StrBufCreate(0x100, UNION_HEAPID);
        unisys->fmtbuf = GFL_StrBufCreate(0x100, UNION_HEAPID);
        UnionMsg_CreateWordSet(unisys);
        unisys->fld_msgwin_stream = MsgBGPrinter_CreatePlainWin(fldmsg_bg, unisys->msgdata, 0, 1, 0x13, 0x1e, 4);
    } else {
        MsgBGPrinter_ClearPlainWin(unisys->fld_msgwin_stream);
    }
}

static void UnionMsg_FreeMsgSystem(UnionSystem *unisys) {
    if (unisys->fld_msgwin_stream != NULL) {
        MsgBGPrinter_OpenPlainWin(unisys->fld_msgwin_stream);
        MsgBGPrinter_DeletePlainWin(unisys->fld_msgwin_stream);
        unisys->fld_msgwin_stream = NULL;
        unisys->msgPrinting = 0;
        GFL_StrBufFree(unisys->msgbuf);
        GFL_StrBufFree(unisys->fmtbuf);
        unisys->msgbuf = NULL;
        unisys->fmtbuf = NULL;
    }
    if (unisys->msgdata != NULL) {
        UnionMsg_FreeWordSet(unisys);
        MsgBGPrinter_DeleteMsgData(unisys->msgdata);
        unisys->msgdata = NULL;
    }
}

void UnionMsg_PrintMessage(UnionSystem *unisys, u32 messageId) {
    GFL_ASSERT(unisys->fld_msgwin_stream != NULL);
    GFL_MsgDataLoadStrbuf(unisys->msgdata, messageId, unisys->msgbuf);
    GFL_WordSetFormatStrbuf(unisys->wordset, unisys->fmtbuf, unisys->msgbuf);
    MsgBGPrinter_PlainWinPrintStream(unisys->fld_msgwin_stream, 0, 0, unisys->fmtbuf);
    unisys->msgPrinting = 1;
}

BOOL UnionMsg_IsPrintDone(UnionSystem *unisys) {
    if (unisys->fld_msgwin_stream == NULL || unisys->msgPrinting == 0) {
        return TRUE;
    }
    return MsgBGPrinter_PlainWinMain(unisys->fld_msgwin_stream);
}

void UnionMsg_CreateAndPrint(UnionSystem *unisys, Field *field, u32 messageId) {
    UnionMsg_CreateMsgSystem(unisys, field);
    UnionMsg_SetupWordSet(unisys);
    UnionMsg_PrintMessage(unisys, messageId);
}

void UnionMsg_CreateYesNo(UnionSystem *unisys, Field *field) {
    if (unisys->fldmenu_yesno_func == NULL) {
        unisys->fldmenu_yesno_func = MsgBGPrinter_CreateYesNoMenu(Field_GetMsgBGSys(field), 0);
    }
}

void UnionMsg_FreeYesNo(UnionSystem *unisys) {
    if (unisys->fldmenu_yesno_func != NULL) {
        MsgBGPrinter_DeleteMenuFunc(unisys->fldmenu_yesno_func);
        unisys->fldmenu_yesno_func = NULL;
    }
}

BOOL UnionMsg_UpdateYesNo(UnionSystem *unisys, BOOL *yes) {
    u32 answer;

    GFL_ASSERT(unisys->fldmenu_yesno_func != NULL);
    answer = MsgBGPrinter_YesNoMenuMain(unisys->fldmenu_yesno_func);
    if (answer == MSGBG_YES_NO_WAITING) {
        return FALSE;
    }
    *yes = answer == MSGBG_YES_NO_YES;
    return TRUE;
}

static void UnionMsg_CreateMenu(UnionSystem *unisys, Field *field, const ListMenuRow *rows, u32 count,
                                ListMenuRequest *request) {
    FieldMsgBG *msgBGSys = Field_GetMsgBGSys(field);

    if (unisys->fldmenu_func == NULL) {
        ListMenuOption *options = MsgBGPrinter_CreateMenuItemsFromTable(rows, count, unisys->msgdata, UNION_HEAPID);

        unisys->fldmenu_func = ListMenuUI_Create(msgBGSys, request, options, NULL, NULL, 0, 0, TRUE);
    }
}

static void UnionMsg_FreeMenu(UnionSystem *unisys) {
    if (unisys->fldmenu_func != NULL) {
        MsgBGPrinter_DeleteMenuFunc(unisys->fldmenu_func);
        unisys->fldmenu_func = NULL;
    }
}

static s32 UnionMsg_UpdateMenu(UnionSystem *unisys, Field *field) {
    void *fldmsg_bg = Field_GetMsgBGSys(field);

    GFL_ASSERT(fldmsg_bg != NULL);
    if (fldmsg_bg != NULL && !MsgBGPrinter_IsPrintQueFinished(fldmsg_bg)) {
        return BMPMENULIST_NULL;
    }
    GFL_ASSERT(unisys->fldmenu_func != NULL);
    return ListMenuUI_Update(unisys->fldmenu_func);
}

void UnionMsg_CreateMainMenu(UnionSystem *unisys, Field *field) {
    UnionMsg_CreateMenu(unisys, field, sMenuMain, 6, (ListMenuRequest *)&sMenuMainRequest);
}

void UnionMsg_FreeMainMenu(UnionSystem *unisys) {
    UnionMsg_FreeMenu(unisys);
}

s32 UnionMsg_UpdateMainMenu(UnionSystem *unisys, Field *field) {
    return UnionMsg_UpdateMenu(unisys, field);
}

// Opens the menu of the battle rules: the kind says which of sMenuInfo's, and which choices the rows show
void UnionMsg_CreateRuleMenu(UnionSystem *unisys, Field *field, int kind, u8 *ruleSel) {
    ListMenuRow *rows;
    u8 count;

    ruleSel[3] = kind;
    ListMenuRequest request = sMenuRulesRequest;
    count = sMenuInfo[kind].count;
    request.count = count;
    request.maxShown = count;
    request.height = request.count * 2;
    if (kind == UNION_RULE_MENU_TWO || kind == UNION_RULE_MENU_FOUR) {
        request.width = 30;
    }
    if (kind >= UNION_RULE_MENU_FOUR) {
        request.y = 5;
    }

    if (kind == UNION_RULE_MENU_TWO) {
        rows = GFL_HeapAllocate(UNION_HEAPID, sizeof(ListMenuRow) * 6, TRUE, "union_msg.c", 0x647);
        MI_CpuCopy8(sMenuTwo, rows, sizeof(ListMenuRow) * 6);
        rows[0].messageId = UnionRoomMessages_Text_BattleFormatSingleBattle + ruleSel[0];
        rows[1].messageId = UnionRoomMessages_Text_BattleRulesNoRestrictions + ruleSel[1];
        rows[2].messageId = UnionRoomMessages_Text_WonderLauncher_2 + ruleSel[2];
        UnionMsg_CreateMenu(unisys, field, rows, count, &request);
        GFL_HeapFree(rows);
    } else if (kind == UNION_RULE_MENU_FOUR) {
        rows = GFL_HeapAllocate(UNION_HEAPID, sizeof(ListMenuRow) * 5, TRUE, "union_msg.c", 0x657);
        MI_CpuCopy8(sMenuFour, rows, sizeof(ListMenuRow) * 5);
        rows[0].messageId = UnionRoomMessages_Text_BattleRulesNoRestrictions + ruleSel[1];
        rows[1].messageId = UnionRoomMessages_Text_WonderLauncher_2 + ruleSel[2];
        UnionMsg_CreateMenu(unisys, field, rows, count, &request);
        GFL_HeapFree(rows);
    } else {
        UnionMsg_CreateMenu(unisys, field, sMenuInfo[kind].rows, count, &request);
    }
}

void UnionMsg_FreeRuleMenu(UnionSystem *unisys) {
    UnionMsg_FreeMenu(unisys);
}

// Whether the number is one of sMenuInfo's menus
static inline BOOL UnionMsg_IsMenuKind(u32 kind) {
    if (kind < NELEMS(sMenuInfo)) {
        return TRUE;
    }
    return FALSE;
}

// The entry of sMenuInfo for a menu kind
static inline const UnionMenuInfo *UnionMsg_GetMenuInfo(int kind) {
    return &sMenuInfo[kind];
}

// Runs the menu of the battle rules and takes the choice: the value is a submenu or a rule. picked says the choice
// changed the rules or opened a menu, isFinal that the rules are confirmed
u32 UnionMsg_UpdateRuleMenu(UnionSystem *unisys, BOOL *picked, u8 *ruleSel, BOOL *isFinal, Field *field) {
    u32 ret;
    u32 i;

    *picked = FALSE;
    *isFinal = FALSE;
    ret = UnionMsg_UpdateMenu(unisys, field);

    switch (ret) {
    case BMPMENULIST_NULL:
        return ret;
    case BMPMENULIST_CANCEL:
        if (ruleSel[3] == UNION_RULE_MENU_PLAYERS) {
            return ret;
        }
        // The last row goes back to the menu that opened this one
        ret = sMenuInfo[ruleSel[3]].rows[UnionMsg_GetMenuInfo(ruleSel[3])->count - 1].value;
        break;
    case UNION_RULE_MENU_CONFIRM:
        return sRuleResult[ruleSel[0]][ruleSel[1]][ruleSel[2]];
    case UNION_RULE_MENU_SHOW_RULES:
        *isFinal = TRUE;
        return sRuleResult[ruleSel[0]][ruleSel[1]][ruleSel[2]];
    }

    if (ruleSel[3] == UNION_RULE_MENU_PLAYERS) {
        if (ret == (u32)sMenuTwo) {
            if (ruleSel[0] == UNION_BATTLE_FORMAT_MULTI) {
                ruleSel[0] = UNION_BATTLE_FORMAT_SINGLE;
            }
        } else if (ret == (u32)sMenuFour) {
            ruleSel[0] = UNION_BATTLE_FORMAT_MULTI;
        }
    }

    for (i = 0; UnionMsg_IsMenuKind(i); i++) {
        if (UnionMsg_GetMenuInfo(i)->rows == (const ListMenuRow *)ret) {
            *picked = TRUE;
            return i;
        }
        switch (ruleSel[3]) {
        case UNION_RULE_MENU_FORMAT:
            if (ret < UNION_BATTLE_FORMAT_MAX) {
                ruleSel[0] = ret;
                *picked = TRUE;
                return UNION_RULE_MENU_TWO;
            }
            break;
        case UNION_RULE_MENU_RULES:
        case UNION_RULE_MENU_FOUR_RULES:
            if (ret < 2) {
                ruleSel[1] = ret;
                *picked = TRUE;
                if (ruleSel[3] == UNION_RULE_MENU_FOUR_RULES) {
                    return UNION_RULE_MENU_FOUR;
                }
                return UNION_RULE_MENU_TWO;
            }
            break;
        case UNION_RULE_MENU_LAUNCHER:
        case UNION_RULE_MENU_FOUR_LAUNCHER:
            if (ret < 2) {
                ruleSel[2] = ret;
                *picked = TRUE;
                if (ruleSel[3] == UNION_RULE_MENU_FOUR_LAUNCHER) {
                    return UNION_RULE_MENU_FOUR;
                }
                return UNION_RULE_MENU_TWO;
            }
            break;
        }
    }
    GFL_ASSERT_MSG(FALSE, "menu_ret=%d\n", ret);
    return 0;
}

void UnionMsg_CreateInfoWindow(UnionSystem *unisys, Field *field) {
    if (unisys->fldmsgwin == NULL) {
        FieldMsgBG *msgBGSys = Field_GetMsgBGSys(field);

        unisys->fldmsgwin = MsgBGPrinter_CreatePlainWin(msgBGSys, unisys->msgdata, 0, 1, 1, 0x1e, 2);
        MsgBGPrinter_PlainWinPrintMsg(unisys->fldmsgwin, 0, 0, UnionRoomMessages_Text_BattleFormatMultiBattle);
    }
}

void UnionMsg_FreeInfoWindow(UnionSystem *unisys) {
    if (unisys->fldmsgwin != NULL) {
        MsgBGPrinter_DeletePlainWin(unisys->fldmsgwin);
        unisys->fldmsgwin = NULL;
    }
}

// Opens the window that lists the regulation's rules, each in the color of whether the party meets it
void UnionMsg_CreateRulesWindow(UnionSystem *unisys, Field *field, u32 okMask, u32 hasLauncher, u32 mode) {
    FieldMsgBG *msgBGSys = Field_GetMsgBGSys(field);
    BOOL launcher;
    int i;

    GFL_ASSERT(unisys->wordset != NULL);
    GFL_ASSERT(unisys->fldmsgwin == NULL);
    GFL_ASSERT(unisys->alloc.rpm == NULL);

    launcher = TRUE;
    if (hasLauncher != 1) {
        launcher = FALSE;
    }
    unisys->fldmsgwin = MsgBGPrinter_CreatePlainWin(msgBGSys, unisys->msgdata, 0, 1, 1, 0x1a, 0x10);
    unisys->alloc.rpm =
        PokeRegulation_CreatePrintMsg(unisys->alloc.regulation, unisys->wordset, UNION_HEAPID, launcher);
    UnionMsg_SetupWordSet(unisys);

    if (mode == 0) {
        StrBuf *title = makeStrbufLoadBattleTypeText(unisys->alloc.regulation, UNION_HEAPID);

        MsgBGPrinter_PlainWinPrintStr(unisys->fldmsgwin, 0, 0, title);
        GFL_StrBufFree(title);
    } else {
        MsgBGPrinter_PlainWinPrintMsg(unisys->fldmsgwin, 0, 0,
                                      mode == 1 ? UnionRoomMessages_Text_Party : UnionRoomMessages_Text_BattleBox);
    }

    for (i = 0; i < 6; i++) {
        RegulationPrintMsg *rpm;
        u16 color;

        if (okMask & (1 << i)) {
            color = PRINT_COLOR(4, 2, 0);
        } else {
            color = PRINT_COLOR(1, 2, 0);
        }

        rpm = unisys->alloc.rpm;
        MsgBGPrinter_PlainWinPrintStrColor(unisys->fldmsgwin, 0, (i * 2 + 4) * 8, rpm->rule[i], color);
        rpm = unisys->alloc.rpm;
        MsgBGPrinter_PlainWinPrintStrColor(unisys->fldmsgwin, 0x70, (i * 2 + 4) * 8, rpm->value[i], color);
    }
}

BOOL UnionMsg_IsRulesPrinted(UnionSystem *unisys, Field *field) {
    void *queue = MsgBGPrinter_GetPrintQue(Field_GetMsgBGSys(field));

    if (unisys->alloc.rpm == NULL) {
        return TRUE;
    }
    return PrintQueue_IsEmpty(queue);
}

void UnionMsg_FreeRulesWindow(UnionSystem *unisys) {
    if (unisys->alloc.rpm != NULL) {
        MsgBGPrinter_DeletePlainWin(unisys->fldmsgwin);
        unisys->fldmsgwin = NULL;
        PokeRegulation_FreePrintMsg(unisys->alloc.rpm);
        unisys->alloc.rpm = NULL;
    }
}
