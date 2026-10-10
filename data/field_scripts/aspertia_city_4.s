#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    VMStackPush 0x40a7
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x40a1
    VMStackPushConst 7
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0045
    FlagSet 803
    VMJump L_0049

L_0045:
    FlagReset 803

L_0049:
    VMStackPushFlag 965
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0083
    VMStackPushFlag 483
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 490
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0083
    FlagReset 965

L_0083:
    VMHalt

Script_2:
    ActorsPauseAll
    WordSetPlayerName 0
    RecordGet 71, 0x400f
    VMStackPush 0x40a1
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00BD
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]Ā\u0001\u0000, did you find\nthat lady called Bianca?[f000]븁\u0000\nI hope you get a Pokémon soon!"
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01E1

L_00BD:
    VMStackPush 0x40a7
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackPush 0x40a1
    VMStackPushConst 8
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0100
    VMCall L_01E7
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I wish...[f000]븁\u0000\nI wish my big brother could go on a\njourney for his Pokémon, not for me."
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01E1

L_0100:
    VMStackPush 0x40a7
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMStackPush 0x40a8
    VMStackPushConst 2
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0143
    VMCall L_01E7
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, [f000]Ā\u0001\u0000!\nTake care of [f000]ā\u0001\u0001![f000]븁\u0000\nOnly Trainers can protect\ntheir own Pokémon!"
    ParentActorMsg MSGFILE_SCRIPT, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01E1

L_0143:
    VMStackPush 0x4124
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C7
    VMCall L_01E7
    VMStackPushFlag 483
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 490
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 965
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_01A9
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "[f000]Ā\u0001\u0000![f000]븁\u0000\nLook![f000]븁\u0000\nLiepard looks so happy\nwhen I pet its head!"
    ParentActorMsg MSGFILE_SCRIPT, 5, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_01C1

L_01A9:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Know what?\nMy big brother told me something.[f000]븁\u0000\nHe said to talk to the Liepard\ninside the Poké Ball lots and lots,[f000]븀\u0000\nlike I'm doing, until it remembers me![f000]븁\u0000\nAnd to pet it, even if it's just on\nthe outside of the Poké Ball!"
    ParentActorMsg MSGFILE_SCRIPT, 4, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet 490

L_01C1:
    VMJump L_01E1

L_01C7:
    VMCall L_01E7
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Hi, [f000]Ā\u0001\u0000![f000]븁\u0000\nWow! It's [f000]ā\u0001\u0001![f000]븁\u0000\nYou know lots of other Pokémon, right?\nCool!"
    ParentActorMsg MSGFILE_SCRIPT, 3, 0, 0
    LastKeyWait
    ActorMsgClose

L_01E1:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_01E7:
    PokePartyGetMemberByType 0x8020, 2
    WordSetPartyPokeSpecies 1, 0x8020
    VMReturn

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 510, 0
    // "Preoooww... ♪"
    ParentActorMsg MSGFILE_SCRIPT, 6, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "It's a map of the Unova region."
    InfoMsg 8, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
