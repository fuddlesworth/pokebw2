#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We were going to make more land,\nbut we realized it would cause[f000]븀\u0000\nproblems for Pokémon living in the sea...[f000]븁\u0000\nOh! Don't tell my girlfriend!"
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "We tried to expand the ocean,\nbut then there would be fewer[f000]븀\u0000\nPokémon that live on land.[f000]븁\u0000\nThat might make the Pokémon in\nthe ocean sad...[f000]븁\u0000\nOh! This is a secret\nfrom my darling boyfriend!"
    ParentActorMsg MSGFILE_SCRIPT, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPush 0x4000
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BA
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I might not look it,\nbut I'm the sound designer![f000]븁\u0000\nI want folks to hear my wonderful music,\nso I'm travelin' all over these parts.[f000]븁\u0000\nHow about givin' my\nfavorite music a listen?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00A6
    // "Well, shucks!\nHave a good listen, then!"
    ParentActorMsg MSGFILE_SCRIPT, 3, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x4000, 1
    BGMPlay SEQ_BGM_R_F
    FlagSet 2559
    BGMAmbienceResume
    VMJump L_00B4

L_00A6:
    // "Oh now, don't be like that!\nListen as much as you want!"
    ParentActorMsg MSGFILE_SCRIPT, 5, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00B4:
    VMJump L_00C8

L_00BA:
    SEPlay SEQ_SE_MESSAGE
    // "He's lost in the music..."
    SystemMsg 4, 2
    LastKeyWait
    InfoMsgClose

L_00C8:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
