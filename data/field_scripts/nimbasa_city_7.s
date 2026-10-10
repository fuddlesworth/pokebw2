#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntry Script_8
    ScriptEntry Script_9
    ScriptEntry Script_10
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
    VMJumpIf CMP_STACK, L_00A5
    StadiumLoadTrainerTable
    Cmd_0249 0x416d, 0x416e, 0x416f, 0x4170, 0x4171, 0x4172
    StadiumFreeTrainerTable

L_00A5:
    FlagSet 649
    FlagSet 650
    FlagSet 651
    FlagSet 652
    FlagSet 653
    TrainerCardHasBadge 0x8020, 4
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D6
    FlagReset 649

L_00D6:
    VMStackPushFlag 2400
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00F9
    FlagReset 650
    FlagReset 651
    FlagReset 652
    FlagReset 653

L_00F9:
    StadiumLoadTrainerTable
    StadiumSetupActorSingle 6, 9, 1
    StadiumSetupActorSingle 7, 10, 1
    StadiumSetupActorSingle 8, 11, 1
    TrainerCardHasBadge 0x8020, 4
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_014E
    StadiumSetupActorSingle 6, 9, 2
    StadiumSetupActorSingle 7, 10, 2
    StadiumSetupActorSingle 8, 11, 2
    StadiumSetupActorsDouble 19, 12, 22, 2

L_014E:
    TrainerCardHasBadge 0x8020, 6
    VMStackPush 0x8020
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0189
    StadiumSetupActorSingle 6, 9, 3
    StadiumSetupActorSingle 7, 10, 3
    StadiumSetupActorSingle 8, 11, 3
    StadiumSetupActorsDouble 19, 12, 22, 3

L_0189:
    VMStackPushFlag 2400
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01D6
    StadiumSetupActorSingle 6, 9, 4
    StadiumSetupActorSingle 7, 10, 4
    StadiumSetupActorSingle 8, 11, 4
    StadiumSetupActorsDouble 19, 12, 22, 4
    StadiumSetupActorsDouble 20, 13, 23, 4
    StadiumSetupActorsTriple 14, 15, 16, 0x416d, 0x416e, 0x416f

L_01D6:
    StadiumFreeTrainerTable
    WorkSetConst 0x8020, 0
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Everybody makes mistakes.[f000]븁\u0000\nBut goalkeepers cannot afford a mistake,\nbecause they cannot score goals to make[f000]븀\u0000\nup for it.[f000]븁\u0000\nIf one Pokémon on a team makes a\nmistake, however, the other Pokémon[f000]븀\u0000\nand their Trainer can cover for it!"
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
    // "If a Striker catches a teammate's eye,\nthey understand each other.[f000]븁\u0000\nA Pokémon and its Trainer are the same.\nDon't you agree?"
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
    // "Really strong Trainers thoroughly do\nwhatever they can do!"
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Soccer is beautiful and fun!\nPokémon are also beautiful and fun!"
    ParentActorMsg MSGFILE_SCRIPT, 3, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Soccer with Pokémon is Pokémon soccer.\nThe abbreviation is...Poker?[f000]븀\u0000\nNo, wait, that's a fireplace tool.[f000]븀\u0000\nHow about Poccer?"
    ParentActorMsg MSGFILE_SCRIPT, 4, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Throw a sound pass to a team member![f000]븁\u0000\nAfter that, to receive a sound pass,\nyou'll need to move swiftly."
    ParentActorMsg MSGFILE_SCRIPT, 5, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Run! Run!\nJust think about running!"
    ParentActorMsg MSGFILE_SCRIPT, 6, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 504, 0
    // "Squeeskwaa!"
    ParentActorMsg MSGFILE_SCRIPT, 7, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 504, 0
    // "Meep! ♪"
    ParentActorMsg MSGFILE_SCRIPT, 8, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
