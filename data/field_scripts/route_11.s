#include "asm/field_script.inc"

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
    VMHalt

Script_6:
    VMStackPush 0x40da
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 801
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_005D
    ActorSetGPos 0, 468, 0, 175, 3

L_005D:
    VMHalt

Script_1:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Route 11"
    MsgPlaceSign 2, 3
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    FunfestMissionBroadcast 17, 0
    SEPlay SEQ_SE_MESSAGE
    // "Village Bridge Ahead"
    MsgPlaceSign 3, 2
    MsgPlaceSignClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    ActorCmdExec 255, Movement_028C
    ActorCmdWait
    EvCameraInit
    EvCameraUnbind
    EvCameraMoveTo 9688, 0, 0xed000, 0x1d53000, 0, 0xa99000, 32
    ActorCmdExec 255, Movement_027C
    ActorCmdWait
    EvCameraWait
    VMSleep 32
    ActorJumpToGPos 0, 468, 0, 173
    EvCameraMoveToDefault 52
    PlayerGetGPos 0x8021, 0x8022
    VMStackPush 0x8022
    VMStackPushConst 173
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0102
    ActorCmdExec 255, Movement_0274
    ActorCmdWait
    VMJump L_0124

L_0102:
    WorkSub 0x8021, 2
    ActorWalkRoute 0, 0x8021, 0x8022, 1, 8, 1
    VMSleep 8
    ActorCmdExec 255, Movement_0274
    ActorCmdWait

L_0124:
    ActorCmdExec 0, Movement_026C
    ActorCmdWait
    EvCameraWait
    EvCameraRebind
    EvCameraEnd
    VMSleep 8
    PVPlay 640, 0
    // "Kikwaaaa!"
    ScreamMsg 0, 1
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    WorkSetConst 0x40da, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 640, 0
    // "Kikwaaaa!"
    ScreamMsg 0, 1
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    VMCall L_017A
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_017A:
    VMStackPushFlag 300
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01AD
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8023, 1
    CallWildBattle 640, 45, 0x8023
    WorkSetConst 0x8023, 0
    VMJump L_01C7

L_01AD:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8024, 1
    CallWildBattle 640, 65, 0x8024
    WorkSetConst 0x8024, 0

L_01C7:
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01F8
    FlagSet 801
    FlagSet 300
    WorkSetConst 0x40da, 1
    ActorDelete 0
    CallWildBattleEnd
    VMJump L_01FA

L_01F8:
    CallWildLose

L_01FA:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0211
    VMJump L_021B

L_0211:
    FlagSet 301
    VMJump L_0241

L_021B:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_023B
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_023B
    VMJump L_0241

L_023B:
    VMJump L_0241

L_0241:
    VMStackPushFlag 301
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_025E
    // "Virizion ran off\ndown the road and vanished..."
    SystemMsg 1, 0
    LastKeyWait
    InfoMsgClose

L_025E:
    VMReturn
    Move 32, 1
    Move 75, 1
    MoveEnd

Movement_026C:
    Move 35, 1
    MoveEnd

Movement_0274:
    Move 34, 1
    MoveEnd

Movement_027C:
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd

Movement_028C:
    Move 75, 1
    MoveEnd
