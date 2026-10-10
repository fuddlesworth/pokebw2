#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMStackPushFlag 920
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x4118
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_004D
    FlagSet 920
    WorkSetConst 0x4118, 1

L_004D:
    VMHalt

Script_2:
    ActorsPauseAll
    // "A mysterious presence can be felt here!\nCheck the surrounding area?"
    SystemMsg 0, 2
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F8
    MsgWinCloseAll
    FlagReset 920
    WorkSetConst 0x4118, 2
    PlayerGetDir 0x8020
    VMStackPush 0x8020
    VMStackPushConst 2
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_009B
    ActorCmdExec 255, Movement_01B0
    ActorCmdWait

L_009B:
    VMSleep 30
    PVPlay 482, 0
    // "Kyuuun!"
    InfoMsg 1, 1
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    PlayerGetGPos 0x8021, 0x8022
    ActorAdd 16
    ActorSetGPos 16, 575, 8, 90, 3
    WorkSub 0x8021, 2
    ActorMoveLinear 16, 0x8021, 6, 0x8022, 32
    ActorSetGPos 16, 0x8021, 6, 0x8022, 2
    ActorCmdExec 16, Movement_01C0
    ActorCmdWait
    VMSleep 16
    VMJump L_00FA

L_00F8:
    MsgWinCloseAll

L_00FA:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 482, 0
    // "Kyuuun!"
    ScreamMsg 1, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    CallWildBattle 482, 65, 1
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_014E
    FlagSet 920
    WorkSetConst 0x4118, 3
    ActorDelete 16
    CallWildBattleEnd
    VMJump L_0150

L_014E:
    CallWildLose

L_0150:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0167
    VMJump L_0171

L_0167:
    FlagSet 398
    VMJump L_01A1

L_0171:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_0191
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_0191
    VMJump L_01A1

L_0191:
    // "Azelf went flying off somewhere..."
    SystemMsg 2, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_01A1

L_01A1:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 35, 1
    MoveEnd

Movement_01B0:
    Move 34, 1
    MoveEnd
    Move 32, 1
    MoveEnd

Movement_01C0:
    Move 33, 1
    MoveEnd
