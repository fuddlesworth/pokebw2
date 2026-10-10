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
    ScriptEntry Script_11
    ScriptEntry Script_12
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Title “White and Black Dragons\""
    InfoMsg 2, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Title “Sin and Money Stack Up\""
    InfoMsg 3, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Title “Ugly Truth\""
    InfoMsg 4, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Title “Vanished Hero\""
    InfoMsg 5, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Title “Eternal Ideals\""
    InfoMsg 6, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Title “Two Beings--A Meeting\nThat Must Occur\""
    InfoMsg 7, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag 2731
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D5
    Random 0x417d, 16
    FlagSet 2731

L_00D5:
    WordSetPokeTypeName 0, 0x417d
    VMStackPushFlag 2732
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03CA
    // "I am not as good an artist as Burgh,\nbut I have a liking for paintings.[f000]븁\u0000\nLet me see. Today, I'd like to paint\n[f000]ă\u0001\u0000-type Pokémon![f000]븁\u0000\nWill you show me what kinds of Pokémon\nyou have?"
    ActorMsg MSGFILE_SCRIPT, 8, 1, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03B4
    // "Please show me a Pokémon that's\n[f000]ă\u0001\u0000 type![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 9, 1, 2, 0
    ActorMsgClose
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    CallPokeSelect 0, 0x8021, 0x8020, 0
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_039E
    WorkSetConst 0x8022, 0
    PokePartyIsEgg 0x8022, 0x8020
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_017C
    // "Oh!\nThe shape of this Egg...[f000]븁\u0000\nSuperb! It gives me chills!\nIt's electrifying! It makes me tremble![f000]븁\u0000\nAlas, even I cannot tell what type\nan Egg is!"
    ActorMsg MSGFILE_SCRIPT, 21, 1, 2, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0398

L_017C:
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    PokePartyGetTypes 0x8023, 0x8024, 0x8020
    VMStackPush 0x417d
    VMStackPush 0x8023
    VMStackCmp CMP_EQ
    VMStackPush 0x417d
    VMStackPush 0x8024
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0388
    // "Oh! You have that type!\nI'm inspired![f000]븁\u0000\nThank you! As a token of my gratitude,\nwhich one would you like?"
    ActorMsg MSGFILE_SCRIPT, 10, 1, 2, 0
    WorkSetConst 0x8025, 0
    ListMenu_AnchorTopRight 31, 1, 0, 0, 32805
    ListMenuAdd 11, 65535, 0
    ListMenuAdd 12, 65535, 1
    ListMenuAdd 13, 65535, 2
    ListMenuAdd 14, 65535, 3
    ListMenuAdd 15, 65535, 4
    ListMenuShow
    VMStackPush 0x8025
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0244
    WordSetItemName 0, 149
    // "One [f000]ĉ\u0001\u0000!\nIt's all yours now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 16, 1, 2, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 149
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_036E

L_0244:
    VMStackPush 0x8025
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0290
    WordSetItemName 0, 150
    // "One [f000]ĉ\u0001\u0000!\nIt's all yours now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 16, 1, 2, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 150
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_036E

L_0290:
    VMStackPush 0x8025
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02DC
    WordSetItemName 0, 151
    // "One [f000]ĉ\u0001\u0000!\nIt's all yours now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 16, 1, 2, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 151
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_036E

L_02DC:
    VMStackPush 0x8025
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0328
    WordSetItemName 0, 152
    // "One [f000]ĉ\u0001\u0000!\nIt's all yours now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 16, 1, 2, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 152
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_036E

L_0328:
    VMStackPush 0x8025
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_036E
    WordSetItemName 0, 153
    // "One [f000]ĉ\u0001\u0000!\nIt's all yours now![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 16, 1, 2, 0
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 153
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000

L_036E:
    // "Aww. You are the best!\nPlease come back tomorrow, too!"
    ActorMsg MSGFILE_SCRIPT, 17, 1, 2, 0
    LastKeyWait
    ActorMsgClose
    FlagSet 2732
    VMJump L_0398

L_0388:
    // "The Pokémon you chose is not the one\nI requested.[f000]븁\u0000\nWhat I want to paint is a Pokémon that's\n[f000]ă\u0001\u0000 type!"
    ActorMsg MSGFILE_SCRIPT, 20, 1, 2, 0
    LastKeyWait
    ActorMsgClose

L_0398:
    VMJump L_03AE

L_039E:
    // "If you catch a Pokémon that's\n[f000]ă\u0001\u0000 type, please show it to me![f000]븀\u0000\nI'll give you a little something."
    ActorMsg MSGFILE_SCRIPT, 19, 1, 2, 0
    LastKeyWait
    ActorMsgClose

L_03AE:
    VMJump L_03C4

L_03B4:
    // "If you catch a Pokémon that's\n[f000]ă\u0001\u0000 type, please show it to me![f000]븀\u0000\nI'll give you a little something."
    ActorMsg MSGFILE_SCRIPT, 19, 1, 2, 0
    LastKeyWait
    ActorMsgClose

L_03C4:
    VMJump L_03DA

L_03CA:
    // "As long as the artist enjoys creating\nan artwork, it doesn't matter if the[f000]븀\u0000\nresult is judged as bad or good."
    ActorMsg MSGFILE_SCRIPT, 18, 1, 2, 0
    LastKeyWait
    ActorMsgClose

L_03DA:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Currently, we're exhibiting works\nfrom up-and-coming young artists!"
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "In Castelia City, the Pokémon Gym\nitself is Burgh's artwork!"
    ParentActorMsg MSGFILE_SCRIPT, 1, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Title “The Recipient\""
    InfoMsg 22, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Title “The Moving Pokémon\""
    InfoMsg 23, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    // "Title “A Gift\""
    InfoMsg 24, 2
    LastKeyWait
    InfoMsgClose_0039
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
