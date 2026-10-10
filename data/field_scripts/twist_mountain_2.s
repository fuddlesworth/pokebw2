#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    RTCGetSeason 0x8020
    FlagSet 682
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0037
    FlagReset 682

L_0037:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "When you walk on snow,\nyour feet sink deeper than usual.[f000]븁\u0000\nThe sensation is so powerful\nand gripping!"
    ParentActorMsg MSGFILE_SCRIPT, 3, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    RTCGetSeason 0x8020
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0088
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Whiteness of snow hides impurities,\nand blackness of night conceals worries."
    ParentActorMsg MSGFILE_SCRIPT, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_009C

L_0088:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "In winter, snow piles up\nall over Twist Mountain.[f000]븁\u0000\nIt gets totally white!"
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_009C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Twist and twist, twisting mountain. ♪\nSpiraling up like a fountain. ♪"
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
