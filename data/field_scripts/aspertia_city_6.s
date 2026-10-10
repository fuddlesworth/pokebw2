#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPushFlag 2401
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0039
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Since Poké Balls were invented,\nanyone can be with Pokémon![f000]븁\u0000\nWe take it for granted now, but\nif you think about it, it's amazing!"
    ParentActorMsg MSGFILE_SCRIPT, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_004D

L_0039:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "What kind of relationship do you want\nwith the Pokémon you meet?[f000]븁\u0000\nI'm happy just having them by my side!"
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_004D:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    VMStackPushFlag 2401
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0086
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hey! Is that your Pokémon?\nWhoa! Cool!"
    // "Hey! Is that your Pokémon?\nWhoa! Cool!"
    ActorMsgGendered 1024, 3, 4, 1, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_009A

L_0086:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I want to have a Pokémon battle soon!"
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0
    LastKeyWait
    ActorMsgClose

L_009A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
