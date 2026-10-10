#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0

Script_1:
    VMStackPush 0x4123
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0033
    FlagReset 966

L_0033:
    VMHalt

Script_2:
    ActorsPauseAll
    PVPlay 571, 0
    // "Kwaaaaan!"
    ActorMsg MSGFILE_SCRIPT, 0, 0, 0, 0
    PVWait
    MsgWaitAdvance
    MsgWinCloseAll
    ActorWalkRoute 0, 4, 7, 1, 6, 1
    ActorCmdWait
    ActorWalkRoute 0, 28, 7, 1, 6, 0
    ActorCmdWait
    ActorDelete 0
    WorkSetConst 0x4123, 4
    FlagSet 966
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd
    Move 35, 1
    MoveEnd
