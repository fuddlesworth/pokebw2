#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntry Script_6
    ScriptEntry Script_7
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag 200
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0049
    // "Ahem![f000]븁\u0000\nI am the chairman who loves Pokémon the\nmost among Pokéfans in the entire world![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 0, 2, 0, 0
    FlagSet 200

L_0049:
    // "If you are a Trainer, will you show me\nhow you are raising your Pokémon[f000]븀\u0000\nwith loving care?"
    ActorMsg MSGFILE_SCRIPT, 1, 2, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0315
    // "Oh!\nWhich Pokémon will you show me?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 3, 2, 0, 0
    ActorMsgClose
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    CallPokeSelect 0, 0x8021, 0x8020, 0
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02FF
    WorkSetConst 0x8022, 0
    PokePartyIsEgg 0x8022, 0x8020
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D8
    // "Well...it's a bit hard to tell how much\nthat Egg has grown."
    ActorMsg MSGFILE_SCRIPT, 5, 2, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_02F9

L_00D8:
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8025, 0
    PokePartyGetParam 0x8023, 0x8020, 153
    PokePartyGetParam 0x8024, 0x8020, 158
    WordSetPartyPokeSpecies 0, 0x8020
    WordSetNumber 1, 0x8023, 3
    WordSetNumber 2, 0x8024, 3
    WorkSub 0x8024, 0x8023
    // "Oh! This [f000]ā\u0001\u0000 was level [f000]Ȃ\u0001\u0001\nwhen you met, but now it's level [f000]Ȃ\u0001\u0002![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 6, 2, 0, 0
    VMStackPush 0x8024
    VMStackPushConst 99
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_019B
    // "You've raised it very well.\nIt's received a lot of love from you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 7, 2, 0, 0
    VMStackPushFlag 203
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0189
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 221
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "That is a token of gratitude for showing\nme your great love for your Pokémon!"
    ActorMsg MSGFILE_SCRIPT, 11, 2, 0, 0
    FlagSet 203
    VMJump L_0195

L_0189:
    // "Well, you showed me good stuff![f000]븁\u0000\nPlease keep raising your Pokémon\nwith loving care!"
    ActorMsg MSGFILE_SCRIPT, 12, 2, 0, 0

L_0195:
    VMJump L_02F5

L_019B:
    VMStackPush 0x8024
    VMStackPushConst 50
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0217
    // "You've raised it quite well.\nI feel your love for this Pokémon.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 8, 2, 0, 0
    VMStackPushFlag 202
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0205
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 224
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "That is a token of gratitude for showing\nme your great love for your Pokémon!"
    ActorMsg MSGFILE_SCRIPT, 11, 2, 0, 0
    FlagSet 202
    VMJump L_0211

L_0205:
    // "Well, you showed me good stuff![f000]븁\u0000\nPlease keep raising your Pokémon\nwith loving care!"
    ActorMsg MSGFILE_SCRIPT, 12, 2, 0, 0

L_0211:
    VMJump L_02F5

L_0217:
    VMStackPush 0x8024
    VMStackPushConst 25
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0293
    // "You've raised it well.\nYou must be affectionate.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 9, 2, 0, 0
    VMStackPushFlag 201
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0281
    ActorMsgClose
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 216
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "That is a token of gratitude for showing\nme your great love for your Pokémon!"
    ActorMsg MSGFILE_SCRIPT, 11, 2, 0, 0
    FlagSet 201
    VMJump L_028D

L_0281:
    // "Well, you showed me good stuff![f000]븁\u0000\nPlease keep raising your Pokémon\nwith loving care!"
    ActorMsg MSGFILE_SCRIPT, 12, 2, 0, 0

L_028D:
    VMJump L_02F5

L_0293:
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B8
    // "What? It has not grown at all.[f000]븁\u0000\nStill, if you travel with your Pokémon\nfrom now on, I am sure it will grow!"
    ActorMsg MSGFILE_SCRIPT, 13, 2, 0, 0
    VMJump L_02F5

L_02B8:
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_02E9
    // "I see! Although it's just a smidgen,\nI can feel your love for your Pokémon.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 10, 2, 0, 0
    // "Well, you showed me good stuff![f000]븁\u0000\nPlease keep raising your Pokémon\nwith loving care!"
    ActorMsg MSGFILE_SCRIPT, 12, 2, 0, 0
    VMJump L_02F5

L_02E9:
    // "...Hmmm.\nIt's hard to tell..."
    ActorMsg MSGFILE_SCRIPT, 4, 2, 0, 0

L_02F5:
    LastKeyWait
    ActorMsgClose

L_02F9:
    VMJump L_030F

L_02FF:
    // "You're a shy Trainer, aren't you?"
    ActorMsg MSGFILE_SCRIPT, 2, 2, 0, 0
    LastKeyWait
    ActorMsgClose

L_030F:
    VMJump L_0325

L_0315:
    // "You're a shy Trainer, aren't you?"
    ActorMsg MSGFILE_SCRIPT, 2, 2, 0, 0
    LastKeyWait
    ActorMsgClose

L_0325:
    WorkSetConst 0x8025, 0
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8020, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Welcome to the Pokémon Fan Club.[f000]븁\u0000\nShall I check how friendly your Pokémon\nis toward you?"
    ActorMsg MSGFILE_SCRIPT, 14, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0519
    ActorMsgClose
    WorkSetConst 0x8026, 0
    WorkSetConst 0x8027, 0
    CallPokeSelect 0, 0x8027, 0x8026, 0
    VMStackPush 0x8027
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0503
    WorkSetConst 0x8028, 0
    PokePartyIsEgg 0x8028, 0x8026
    VMStackPush 0x8028
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_03DA
    // "I can't tell whether or not you and\nthe Egg are close friends."
    ActorMsg MSGFILE_SCRIPT, 15, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04FD

L_03DA:
    WordSetPartyPokeSpecies 0, 0x8026
    // "Oh, my. Your [f000]ā\u0001\u0000...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 16, 0, 0, 0
    WorkSetConst 0x8029, 0
    PokePartyGetHappiness 0x8029, 0x8026
    VMStackPush 0x8029
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0420
    // "By any chance, you...[f000]븁\u0000\nAre you a very strict person?\nI feel that it really doesn't like you..."
    ActorMsg MSGFILE_SCRIPT, 23, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04FD

L_0420:
    VMStackPush 0x8029
    VMStackPushConst 255
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0449
    // "It is super friendly to you!\nI'm a bit jealous!"
    ActorMsg MSGFILE_SCRIPT, 17, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04FD

L_0449:
    VMStackPush 0x8029
    VMStackPushConst 200
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_0472
    // "It is quite friendly to you!\nYou must be a kind person!"
    ActorMsg MSGFILE_SCRIPT, 18, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04FD

L_0472:
    VMStackPush 0x8029
    VMStackPushConst 150
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_049B
    // "It is friendly to you.\nIt must be happy with you."
    ActorMsg MSGFILE_SCRIPT, 19, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04FD

L_049B:
    VMStackPush 0x8029
    VMStackPushConst 100
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_04C4
    // "It is a little friendly to you...\nThat's what I'm getting."
    ActorMsg MSGFILE_SCRIPT, 20, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04FD

L_04C4:
    VMStackPush 0x8029
    VMStackPushConst 50
    VMStackCmp CMP_GE
    VMJumpIf CMP_STACK, L_04ED
    // "The relationship is neither good\nnor bad... It looks neutral."
    ActorMsg MSGFILE_SCRIPT, 21, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_04FD

L_04ED:
    // "Hmmm...\nIt may not like you very much."
    ActorMsg MSGFILE_SCRIPT, 22, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_04FD:
    VMJump L_0513

L_0503:
    // "Oh, you are so shy! Come on,\ndon't hide your Pokémon from me."
    ActorMsg MSGFILE_SCRIPT, 24, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0513:
    VMJump L_0529

L_0519:
    // "Oh, you are so shy! Come on,\ndon't hide your Pokémon from me."
    ActorMsg MSGFILE_SCRIPT, 24, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0529:
    WorkSetConst 0x8029, 0
    WorkSetConst 0x8028, 0
    WorkSetConst 0x8027, 0
    WorkSetConst 0x8026, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 517, 0
    // "Muuun!"
    ParentActorMsg MSGFILE_SCRIPT, 25, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 552, 0
    // "Glibalugga!"
    ParentActorMsg MSGFILE_SCRIPT, 26, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 531, 0
    // "Dii?"
    ParentActorMsg MSGFILE_SCRIPT, 27, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 580, 0
    // "Quaa!"
    ParentActorMsg MSGFILE_SCRIPT, 28, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 524, 0
    // "Rola."
    ParentActorMsg MSGFILE_SCRIPT, 29, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
