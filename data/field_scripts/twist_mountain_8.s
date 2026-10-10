#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    SEPlay SEQ_SE_MESSAGE
    PokePartyGetCountBySpecies 378, 0x8020
    PokePartyGetCountBySpecies 377, 0x8021
    PokePartyGetCountBySpecies 379, 0x8022
    VMStackPush 0x8020
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_007F
    // "It's a statue of a Pokémon.\nIt exudes tremendous power..."
    SystemMsg 0, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_0085

L_007F:
    VMCall L_008B

L_0085:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_008B:
    // "It's a statue of a Pokémon.\nIt exudes tremendous power...[f000]븁\u0000\n..."
    SystemMsg 1, 2
    MsgWaitAdvance
    InfoMsgClose
    PVPlay 486, 0
    // "...Zut zutt!"
    ScreamMsg 2, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallWildBattle 486, 68, 1
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D5
    FlagSet 924
    ActorDelete 3
    CallWildBattleEnd
    VMJump L_00D7

L_00D5:
    CallWildLose

L_00D7:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_00EE
    VMJump L_00F8

L_00EE:
    FlagSet 402
    VMJump L_0128

L_00F8:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0118
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0118
    VMJump L_0128

L_0118:
    // "Regigigas disappeared\nsomewhere into the passage..."
    SystemMsg 3, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_0128

L_0128:
    VMReturn

Script_2:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    // "[f000]Ā\u0001\u0000 read the\nengraved writing...[f000]븁\u0000\n“A body of rock.\nTo summon the king,[f000]븀\u0000\nsuch a thing must be obtained...\""
    InfoMsg 4, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    // "[f000]Ā\u0001\u0000 read the\nengraved writing...[f000]븁\u0000\n“A body of ice.\nTo summon the king,[f000]븀\u0000\nsuch a thing must be obtained...\""
    InfoMsg 5, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WordSetPlayerName 0
    SEPlay SEQ_SE_MESSAGE
    // "[f000]Ā\u0001\u0000 read the\nengraved writing...[f000]븁\u0000\n“A body of steel.\nTo summon the king,[f000]븀\u0000\nsuch a thing must be obtained...\""
    InfoMsg 6, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
