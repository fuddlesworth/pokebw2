#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntriesEnd

Script_1:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_FLD_68
    SEWait
    // "Click![f000]븁\u0000\nThe sound reverberates."
    InfoMsg 0, 2
    LastKeyWait
    InfoMsgClose_0039
    FlagReset 215
    FlagReset 214
    WorkSetConst 0x4094, 0
    WorkSetConst 0x4095, 0
    WorkSetConst 0x4096, 0
    ActorSetGPos 0, 80, 3, 79, 1
    ActorSetGPos 1, 80, 3, 77, 1
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_FLD_153
    SEWait
    // "A dull sound came from far away."
    InfoMsg 1, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_FLD_154
    EvCameraShake 5, 0, 3, 8, 1, 0, 1, 5
    SEWait
    // "A dull sound echoed."
    InfoMsg 2, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_FLD_155
    EvCameraShake 8, 0, 3, 15, 1, 0, 1, 5
    SEWait
    // "The dull sound is close!"
    InfoMsg 3, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    // "It's a torrent of water!"
    InfoMsg 4, 2
    LastKeyWait
    InfoMsgClose_0039
    SEPlay SEQ_SE_FLD_156
    EvCameraShake 10, 0, 3, 20, 1, 0, 1, 5
    SEWait
    CallDiving 2
    FlagReset 215
    FlagReset 214
    WorkSetConst 0x4094, 0
    WorkSetConst 0x4095, 0
    WorkSetConst 0x4096, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    VMStackPushFlag 215
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_016F
    EvCameraShake 0, 1, 3, 6, 1, 0, 1, 5
    SEPlay SEQ_SE_FLD_126
    ActorCmdExec 0, Movement_01D0
    ActorCmdExec 1, Movement_01D0
    ActorCmdWait
    SEWait
    // "The wall moved, and you can proceed now!"
    InfoMsg 5, 2
    LastKeyWait
    InfoMsgClose_0039
    FlagSet 215
    WorkSetConst 0x4095, 1

L_016F:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    VMStackPushFlag 214
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C7
    EvCameraShake 0, 1, 3, 6, 1, 0, 1, 5
    SEPlay SEQ_SE_FLD_126
    ActorCmdExec 0, Movement_01DC
    ActorCmdExec 1, Movement_01DC
    ActorCmdWait
    SEWait
    // "The wall moved, and you can proceed now!"
    InfoMsg 5, 2
    LastKeyWait
    InfoMsgClose_0039
    FlagSet 214
    WorkSetConst 0x4096, 1

L_01C7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_01D0:
    Move 4, 1
    Move 8, 2
    MoveEnd

Movement_01DC:
    Move 7, 1
    Move 11, 2
    MoveEnd
