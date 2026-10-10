#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPushFlag 2406
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_003D
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Aspertia City is in the corner of the\nUnova region, so this isn't exactly the[f000]븀\u0000\nbig leagues for Pokémon battling.[f000]븁\u0000\nIf you go to the center of Unova,\nthere are many Pokémon Trainers[f000]븀\u0000\nyou can battle."
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0051

L_003D:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wonder if everyone will change now\nthat we have a Pokémon Gym?[f000]븁\u0000\nI mean, if your friends get good at\nbattling, you'll want to get better[f000]븀\u0000\nalso, right?"
    ParentActorMsg MSGFILE_SCRIPT, 1, 0, 0
    LastKeyWait
    ActorMsgClose

L_0051:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    TrainerCardHasBadge 0x8010, 0
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_008C
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm going to try battling\nwith my Pokémon...[f000]븁\u0000\nIf I don't do anything because I'm afraid\nit will get hurt, it will never get strong."
    ParentActorMsg MSGFILE_SCRIPT, 3, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_00A0

L_008C:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I haven't battled with my Pokémon,\nso it's still weak..."
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0
    LastKeyWait
    ActorMsgClose

L_00A0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 506, 0
    // "Yap! Yap yap!"
    ParentActorMsg MSGFILE_SCRIPT, 4, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
