#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    Cmd_017A 36
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to Opelucid City,\nwhere people value history![f000]븁\u0000\n...Have you heard this before?"
    // "Welcome to Opelucid City,\nwhere people like changes![f000]븁\u0000\n...Have you heard this before?"
    ActorMsgVersioned 1024, 1, 0, 0, 0, 0
    LastKeyWait
    MsgWinCloseAll
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPushFlag 2400
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0069
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Munch, munch...\nI am happy when I am eating![f000]븁\u0000\nMunch, munch...\nBut when I don't get food, I am angry![f000]븁\u0000\nThe words hungry and angry are similar.\nDon't you think?"
    ParentActorMsg MSGFILE_SCRIPT, 4, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0081

L_0069:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Munch, munch...\nMaybe about two years ago now...[f000]븁\u0000\nI heard that a guy with\nthe legendary Pokémon Reshiram[f000]븀\u0000\nvisited Opelucid City...[f000]븁\u0000\nMunch, munch..."
    // "Munch, munch...\nMaybe about two years ago now...[f000]븁\u0000\nI heard that a guy with\nthe legendary Pokémon Zekrom[f000]븀\u0000\nvisited Opelucid City...[f000]븁\u0000\nMunch, munch..."
    ActorMsgVersioned 1024, 3, 2, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0081:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
