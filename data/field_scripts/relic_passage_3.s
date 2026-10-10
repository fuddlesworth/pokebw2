#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    VMStackPushFlag 2458
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0050
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm here collecting Shards so that\nI can have my Pokémon be taught moves.[f000]븁\u0000\nThey can be found in the dust clouds,\nbut rarely you'll find a Pokémon instead."
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    MsgWaitAdvance
    MsgWinCloseAll
    Cmd_0275 0, 15, 0
    SEPlay SEQ_SE_FLD_133
    // "The Funfest Mission\n“[f000]ŀ\u0001\u0000\"[f000]븀\u0000\nhas been added to the Entralink."
    SystemMsg 1, 0
    SEWait
    LastKeyWait
    MsgWinCloseAll
    FlagSet 2458
    VMJump L_0064

L_0050:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I'm here collecting Shards so that\nI can have my Pokémon be taught moves.[f000]븁\u0000\nThey can be found in the dust clouds,\nbut rarely you'll find a Pokémon instead."
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0064:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
