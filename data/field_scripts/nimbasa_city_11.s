#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd

Script_1:
    WorkSetConst 0x8020, 0
    VMStackPush 0x416d
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x416e
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x416f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x4170
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x4171
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x4172
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_008D
    StadiumLoadTrainerTable
    Cmd_0249 0x416d, 0x416e, 0x416f, 0x4170, 0x4171, 0x4172
    StadiumFreeTrainerTable

L_008D:
    FlagSet 649
    FlagSet 650
    FlagSet 651
    FlagSet 652
    FlagSet 653
    TrainerCardHasBadge 0x8020, 4
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BE
    FlagReset 649

L_00BE:
    VMStackPushFlag 2400
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E1
    FlagReset 650
    FlagReset 651
    FlagReset 652
    FlagReset 653

L_00E1:
    StadiumLoadTrainerTable
    StadiumSetupActorSingle 12, 0, 1
    StadiumSetupActorSingle 13, 1, 1
    StadiumSetupActorSingle 14, 2, 1
    TrainerCardHasBadge 0x8020, 4
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0136
    StadiumSetupActorSingle 12, 0, 2
    StadiumSetupActorSingle 13, 1, 2
    StadiumSetupActorSingle 14, 2, 2
    StadiumSetupActorsDouble 23, 3, 10, 2

L_0136:
    TrainerCardHasBadge 0x8020, 6
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0171
    StadiumSetupActorSingle 12, 0, 3
    StadiumSetupActorSingle 13, 1, 3
    StadiumSetupActorSingle 14, 2, 3
    StadiumSetupActorsDouble 23, 3, 10, 3

L_0171:
    VMStackPushFlag 2400
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01BE
    StadiumSetupActorSingle 12, 0, 4
    StadiumSetupActorSingle 13, 1, 4
    StadiumSetupActorSingle 14, 2, 4
    StadiumSetupActorsDouble 23, 3, 10, 4
    StadiumSetupActorsDouble 24, 4, 11, 4
    StadiumSetupActorsTriple 5, 6, 7, 0x4170, 0x4171, 0x4172

L_01BE:
    StadiumFreeTrainerTable
    WorkSetConst 0x8020, 0
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "A fight against your opponent can end in\nthe blink of an eye![f000]븁\u0000\nThe important thing is how much you\nprepare before the fight."
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I don't play basketball just because\nI'm tall.[f000]븁\u0000\nMaybe I grew tall because I wanted to\nplay basketball so badly?!"
    ParentActorMsg MSGFILE_SCRIPT, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 559, 0
    // "Aaagy!"
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
