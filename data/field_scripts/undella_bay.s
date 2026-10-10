#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0

Script_1:
    RTCGetWeekDay 0x8023
    GameGetVersion 0x8024
    VMStackPush 0x8024
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 2775
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0077
    FlagReset 903
    WorkSetConst 0x4020, 323
    VMJump L_00BE

L_0077:
    VMStackPush 0x8024
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackPushFlag 2775
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00BA
    FlagReset 903
    WorkSetConst 0x4020, 324
    VMJump L_00BE

L_00BA:
    FlagSet 903

L_00BE:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    GameGetVersion 0x8024
    VMStackPush 0x8024
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_011C
    PVPlay 593, 0
    // "Jelliiii!"
    ScreamMsg 0, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8025, 128
    WorkOr 0x8025, 8
    WorkOr 0x8025, 32
    CallWildBattle 593, 40, 0x8025
    WorkSetConst 0x8025, 0
    VMJump L_0166

L_011C:
    VMStackPush 0x8024
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0166
    PVPlay 593, 0
    // "Jeeelliii. ♪"
    ScreamMsg 2, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8026, 128
    WorkOr 0x8026, 8
    WorkOr 0x8026, 64
    CallWildBattle 593, 40, 0x8026
    WorkSetConst 0x8026, 0

L_0166:
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0191
    FlagSet 903
    FlagSet 2775
    ActorDelete 8
    CallWildBattleEnd
    VMJump L_0193

L_0191:
    CallWildLose

L_0193:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_01AA
    VMJump L_01B0

L_01AA:
    VMJump L_021A

L_01B0:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_01D0
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_01D0
    VMJump L_021A

L_01D0:
    GameGetVersion 0x8024
    VMStackPush 0x8024
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F7
    // "Jellicent dove down into\nthe depths of the ocean..."
    SystemMsg 1, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_0214

L_01F7:
    VMStackPush 0x8024
    VMStackPushConst 22
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0214
    // "Jellicent dove down into\nthe depths of the ocean..."
    SystemMsg 3, 2
    LastKeyWait
    InfoMsgClose

L_0214:
    VMJump L_021A

L_021A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    MEPlay SEQ_ME_CALL
    // "The Xtransceiver is ringing."
    SystemMsg 4, 2
    MEWait
    WordSetPlayerName 0
    // "[f000]Ā\u0001\u0000 picked up the Xtransceiver.[f000]븁\u0000"
    SystemMsg 5, 2
    MsgWinCloseAll
    FadeOutBlackQ
    FadeWait
    CallXTransceiver 8, 0
    FadeInBlackQ
    FadeWait
    WorkSetConst 0x4146, 2
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
