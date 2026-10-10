#include "asm/field_script.inc"

// Script plugin 14, from the zones that start its scripts

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    WorkSetConst 0x8008, 13
    WorkAdd 0x8008, 0x4181
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorMsg MSGFILE_SCRIPT, 0x8008, 254, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMStackPush 0x4181
    VMStackPushConst 2
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0073
    WorkAdd 0x4181, 1

L_0073:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    WordSetLoadRivalName 1
    WorkSetConst 0x8008, 23
    WorkAdd 0x8008, 0x4174
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorMsg MSGFILE_SCRIPT, 0x8008, 254, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMStackPush 0x4174
    VMStackPushConst 4
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_00C0
    WorkAdd 0x4174, 1

L_00C0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    RTGetZoneID 0x400f
    VMStackPush 0x400f
    VMStackPushConst 273
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F9
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]븉\u0001\u0001I remember...[f000]븁\u0000\nThis is the place where Anthea and\nConcordia took care of me as a human.[f000]븁\u0000\nThe Seven Sages all taught\nme many different things...[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, 11, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_010D

L_00F9:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]븉\u0001\u0001I...want to see things no one can see.[f000]븉\u0001\u0000"
    ParentActorMsg MSGFILE_SCRIPT, 12, 0, 0
    LastKeyWait
    ActorMsgClose

L_010D:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    WordSetPlayerName 0
    WorkSetConst 0x8008, 2
    WorkAdd 0x8008, 0x4195
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    ActorMsg MSGFILE_SCRIPT, 0x8008, 254, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMStackPush 0x4195
    VMStackPushConst 4
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0157
    WorkAdd 0x4195, 1

L_0157:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_1:
    VMStackPushFlag 2406
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0172
    RTEndGlobal

L_0172:
    ActorPairSet 0x8000, 0x8001, 0x8002, 0x8003, 0x8004
    Cmd_022E 254
    RTEndGlobal
    VMHalt

Script_2:
    VMStackPushFlag 2406
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_019B
    RTEndGlobal

L_019B:
    ActorPairEnd 0x8000, 0x8001
    RTEndGlobal
    VMHalt
    .balign 4, 0
    Move 13, 1
    MoveEnd
    Move 12, 1
    MoveEnd
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd
    VMHalt
    .byte 0x01
    .byte 0x00
    .byte 0xfe
    .balign 4, 0
    Move 3, 1
    MoveEnd
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
    Move 75, 1
    MoveEnd
