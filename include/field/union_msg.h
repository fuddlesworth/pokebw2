#ifndef POKEBW2_FIELD_UNION_MSG_H
#define POKEBW2_FIELD_UNION_MSG_H

#include "types.h"
#include "battle/regulation.h"
#include "field/field.h"
#include "field/union_comm.h"
#include "struct_decls.h"

// union_msg.c, overlay 34, which the ROM names: the Union Room's messages, menus and their tables. Names are not
// swan's. The first functions are lookups of message IDs, by gender, language or a random pick

// Main code that overlay 34 is the only user of yet
StrBuf *makeStrbufLoadBattleTypeText(Regulation *regulation, HeapID heapId);

// The message IDs of the Union Room's texts, for a gender (0 or 1) or a language. The tables are of two IDs
u16 UnionMsg_GetNoReplyMessage(UnionSystem *unisys);
u16 UnionMsg_GetContactGreetingMessage(u8 gender);
u16 UnionMsg_GetJustAskMessage(u8 gender);
u16 UnionMsg_GetBusyMessage(u8 gender);
u16 UnionMsg_GetShowCardMessage(unsigned int row, u8 gender);
u16 UnionMsg_GetCardMissingMessage(unsigned int row, u8 gender);
u16 UnionMsg_GetOfferMessage(unsigned int row);
u16 UnionMsg_GetDeclinedMessage(unsigned int row);
u16 UnionMsg_GetWhatToDoMessage(u8 gender);
u16 UnionMsg_GetBattleStartMessage(u8 gender);
u16 UnionMsg_GetOnePokemonCannotTradeMessage(u8 gender);
u16 UnionMsg_GetNeedEggMessage(u8 gender);
u16 UnionMsg_GetEggCannotTradeMessage(UnionSystem *unisys);
u16 UnionMsg_GetEggCannotTradeResultMessage(UnionSystem *unisys);
u16 UnionMsg_GetBattleChatMessage(u8 gender);
u16 UnionMsg_GetBusyTrainerMessage(u8 gender);
u16 UnionMsg_GetCardChatMessage(u8 gender);
u16 UnionMsg_GetDrawChatMessage(u8 gender);
u16 UnionMsg_GetTradeChatMessage(u8 gender);
u16 UnionMsg_GetSpinTradeChatMessage(u8 gender);
u16 UnionMsg_GetJoinMultiBattleMessage(u8 gender);
u16 UnionMsg_GetJoinSpinTradeMessage(u8 gender);
u16 UnionMsg_GetJoinDrawMessage(u8 gender);
u16 UnionMsg_GetBattleDeclineMessage(u8 gender);
u16 UnionMsg_GetEggTradeDeclineMessage(u8 gender);
u16 UnionMsg_GetDrawDeclineMessage(u8 gender);
u16 UnionMsg_GetNoEggMessage(u8 gender);
u16 UnionMsg_GetTargetEggCannotTradeMessage(u8 gender);
u16 UnionMsg_GetEggTradeAcceptMessage(u8 gender);
u16 UnionMsg_GetSpinTradeStartMessage(u8 gender);
u16 UnionMsg_GetEggTradeBusyMessage(u8 gender);
u16 UnionMsg_GetDrawAcceptMessage(u8 gender);
u16 UnionMsg_GetDrawStartMessage(u8 gender);
u16 UnionMsg_GetDrawBusyMessage(u8 gender);
u16 UnionMsg_GetBattleAcceptMessage(u8 gender);
u16 UnionMsg_GetMultiBattleStartMessage(u8 gender);
u16 UnionMsg_GetBattleBusyMessage(u8 gender);
// The message of a trainer card's origin: nothing, another country, another province or the same
u32 UnionMsg_GetOriginMessage(UnionSystem *unisys, u8 country, u8 province);
// The most players an activity takes: 2, 4 for a multi battle, 5 for drawing and the spin trade (0 past the table)
s32 UnionMsg_GetActivityPlayerCount(s32 index);
// Frees the message system, the menus and the windows
void UnionMsg_FreeAll(UnionSystem *unisys);
// Creates the message system (the message data, the stream window, the word set) or reopens its window
void UnionMsg_CreateMsgSystem(UnionSystem *unisys, Field *field);
void UnionMsg_PrintMessage(UnionSystem *unisys, u32 messageId);
BOOL UnionMsg_IsPrintDone(UnionSystem *unisys);
// Creates the message system, sets the words and prints a message
void UnionMsg_CreateAndPrint(UnionSystem *unisys, Field *field, u32 messageId);
void UnionMsg_CreateYesNo(UnionSystem *unisys, Field *field);
void UnionMsg_FreeYesNo(UnionSystem *unisys);
BOOL UnionMsg_UpdateYesNo(UnionSystem *unisys, BOOL *yes);
void UnionMsg_CreateMainMenu(UnionSystem *unisys, Field *field);
void UnionMsg_FreeMainMenu(UnionSystem *unisys);
s32 UnionMsg_UpdateMainMenu(UnionSystem *unisys, Field *field);
// The menus of the battle rules, the kinds of UnionMsg_CreateRuleMenu: the number of players, then for two players
// the summary and its format, rules and Wonder Launcher submenus, and for four the summary and its rules and Wonder
// Launcher submenus
enum {
    UNION_RULE_MENU_PLAYERS,
    UNION_RULE_MENU_TWO,
    UNION_RULE_MENU_FORMAT,
    UNION_RULE_MENU_RULES,
    UNION_RULE_MENU_LAUNCHER,
    UNION_RULE_MENU_FOUR,
    UNION_RULE_MENU_FOUR_RULES,
    UNION_RULE_MENU_FOUR_LAUNCHER,
};

void UnionMsg_CreateRuleMenu(UnionSystem *unisys, Field *field, int kind, u8 *ruleSel);
void UnionMsg_FreeRuleMenu(UnionSystem *unisys);
u32 UnionMsg_UpdateRuleMenu(UnionSystem *unisys, BOOL *picked, u8 *ruleSel, BOOL *isFinal, Field *field);
void UnionMsg_CreateInfoWindow(UnionSystem *unisys, Field *field);
void UnionMsg_FreeInfoWindow(UnionSystem *unisys);
// The regulation's rules in a window, with the Wonder Launcher when hasLauncher is 1, under its title (mode 0), or
// with the party (1) or the Battle Box (2) checked against them
void UnionMsg_CreateRulesWindow(UnionSystem *unisys, Field *field, u32 okMask, u32 hasLauncher, u32 mode);
BOOL UnionMsg_IsRulesPrinted(UnionSystem *unisys, Field *field);
void UnionMsg_FreeRulesWindow(UnionSystem *unisys);

#endif // POKEBW2_FIELD_UNION_MSG_H
