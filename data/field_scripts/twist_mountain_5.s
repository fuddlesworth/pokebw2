#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd

Script_2:
    FlagSet 690
    WorkSetConst 0x8020, 0
    RTCGetSeason 0x8020
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0033
    FlagReset 690

L_0033:
    WorkSetConst 0x8020, 0
    VMHalt

Script_1:
    ActorsPauseAll
    VMStackPushFlag 2400
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_006A
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you look for something in an empty\nplace like this, you can discover things!"
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_024B

L_006A:
    VMStackPushFlag 2400
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPushFlag 2747
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0237
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you look for something in an empty\nplace like this, you can discover things![f000]븁\u0000\nLike this Fossil I just found! Take this![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 1, 2, 0, 0
    ActorMsgClose
    Random 0x400f, 7
    VMStackPush 0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00E0
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 99
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_021D

L_00E0:
    VMStackPush 0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0119
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 100
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_021D

L_0119:
    VMStackPush 0x400f
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0152
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 101
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_021D

L_0152:
    VMStackPush 0x400f
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_018B
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 102
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_021D

L_018B:
    VMStackPush 0x400f
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C4
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 103
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_021D

L_01C4:
    VMStackPush 0x400f
    VMStackPushConst 5
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01FD
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 104
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_021D

L_01FD:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 105
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000

L_021D:
    // "If you come again tomorrow, you might be\nable to find a Fossil of your own.[f000]븀\u0000\nSo come on out and play if ya want!"
    ActorMsg MSGFILE_SCRIPT, 2, 2, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet 2747
    VMJump L_024B

L_0237:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "If you come again tomorrow, you might be\nable to find a Fossil of your own.[f000]븀\u0000\nSo come on out and play if ya want!"
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0
    LastKeyWait
    ActorMsgClose

L_024B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x40fe
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02EA
    WorkSetConst 0x40fe, 1
    // "Hello!\nI'm a Heavy Machinery Pro![f000]븁\u0000\nAnd... You!\nDo you like construction trucks?"
    ActorMsg MSGFILE_SCRIPT, 3, 7, 2, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02D0

L_02A7:
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02D0
    // "What? Sorry, I couldn't hear you.[f000]븁\u0000\nI'll ask you again!\nDo you like construction trucks?"
    ActorMsg MSGFILE_SCRIPT, 5, 7, 2, 0
    YesNoWin 0x8010
    VMJump L_02A7

L_02D0:
    // "Oh! I knew it!\nConstruction trucks are cool, right?[f000]븁\u0000\nNow, I'll give you a quiz![f000]븁\u0000\nThere are five questions in total!\nIf you answer all of them correctly,[f000]븀\u0000\nI may give you a present!"
    ActorMsg MSGFILE_SCRIPT, 4, 7, 2, 0
    MsgWaitAdvance
    VMCall L_046C
    VMJump L_0466

L_02EA:
    VMStackPush 0x40fe
    VMStackPushConst 1
    VMStackCmp CMP_GE
    VMStackPush 0x40fe
    VMStackPushConst 5
    VMStackCmp CMP_LE
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_040E
    VMStackPushFlag 2769
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0336
    // "Yay, a future Heavy Machinery Pro![f000]븁\u0000\nIf you come see me again tomorrow,\nI'll give you the next question.[f000]븀\u0000\nSee you then!"
    ActorMsg MSGFILE_SCRIPT, 34, 7, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0408

L_0336:
    VMStackPushFlag 2770
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_035F
    // "If you come back tomorrow,\nyou can give that question another try.[f000]븁\u0000\nI'll be waiting for you!"
    ActorMsg MSGFILE_SCRIPT, 35, 7, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0408

L_035F:
    // "Hi! I've been waiting for you!\nOK. Let's get started!"
    ActorMsg MSGFILE_SCRIPT, 6, 7, 2, 0
    MsgWaitAdvance
    WorkCmpConst 0x40fe, 1
    VMJumpIf CMP_EQ, L_0380
    VMJump L_038C

L_0380:
    VMCall L_046C
    VMJump L_0408

L_038C:
    WorkCmpConst 0x40fe, 2
    VMJumpIf CMP_EQ, L_039F
    VMJump L_03AB

L_039F:
    VMCall L_04EE
    VMJump L_0408

L_03AB:
    WorkCmpConst 0x40fe, 3
    VMJumpIf CMP_EQ, L_03BE
    VMJump L_03CA

L_03BE:
    VMCall L_0570
    VMJump L_0408

L_03CA:
    WorkCmpConst 0x40fe, 4
    VMJumpIf CMP_EQ, L_03DD
    VMJump L_03E9

L_03DD:
    VMCall L_05F2
    VMJump L_0408

L_03E9:
    WorkCmpConst 0x40fe, 5
    VMJumpIf CMP_EQ, L_03FC
    VMJump L_0408

L_03FC:
    VMCall L_0674
    VMJump L_0408

L_0408:
    VMJump L_0466

L_040E:
    VMStackPush 0x40fe
    VMStackPushConst 6
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0466
    WorkSetConst 0x8024, 0
    MedalIsObtained 0x8024, 99
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0456
    // "I'll tell Mr. Medal about your talent\nas a Heavy Machinery Pro."
    ActorMsg MSGFILE_SCRIPT, 37, 7, 2, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_0466

L_0456:
    // "I'm so happy that I witnessed the\ncrowning of a Heavy Machinery Pro[f000]븀\u0000\nof a new generation!"
    ActorMsg MSGFILE_SCRIPT, 38, 7, 2, 0
    LastKeyWait
    MsgWinCloseAll

L_0466:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_046C:
    // "Construction Truck Quiz!\nFor short: TruQ![f000]븁\u0000\nNow, here's the question![f000]븁\u0000\nWhich place is famous for\nan old rusty crane truck?"
    ActorMsg MSGFILE_SCRIPT, 7, 7, 2, 0
    MsgWaitAdvance
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32801
    ListMenuAdd 8, 65535, 0
    ListMenuAdd 9, 65535, 1
    ListMenuAdd 10, 65535, 2
    ListMenuAdd 11, 65535, 3
    ListMenuShow
    VMStackPush 0x8021
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_04D8
    // "...[f000]븁\u0000\nCorrect!\nWell done![f000]븁\u0000\nYou're sharp!"
    ActorMsg MSGFILE_SCRIPT, 32, 7, 2, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet 2769
    WorkSetConst 0x40fe, 2
    VMJump L_04EC

L_04D8:
    // "...[f000]븁\u0000\nHmm... Close!\nToo bad![f000]븁\u0000\nBut you were on the right track!"
    ActorMsg MSGFILE_SCRIPT, 33, 7, 2, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet 2770

L_04EC:
    VMReturn

L_04EE:
    // "Construction Truck Quiz!\nFor short: TruQ![f000]븁\u0000\nNow, here's the question![f000]븁\u0000\nThe trucks that run on Route 4\ncome in three different colors:[f000]븀\u0000\nred, blue, and...what?"
    ActorMsg MSGFILE_SCRIPT, 12, 7, 2, 0
    MsgWaitAdvance
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32801
    ListMenuAdd 13, 65535, 0
    ListMenuAdd 14, 65535, 1
    ListMenuAdd 15, 65535, 2
    ListMenuAdd 16, 65535, 3
    ListMenuShow
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_055A
    // "...[f000]븁\u0000\nCorrect!\nWell done![f000]븁\u0000\nYou're sharp!"
    ActorMsg MSGFILE_SCRIPT, 32, 7, 2, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet 2769
    WorkSetConst 0x40fe, 3
    VMJump L_056E

L_055A:
    // "...[f000]븁\u0000\nHmm... Close!\nToo bad![f000]븁\u0000\nBut you were on the right track!"
    ActorMsg MSGFILE_SCRIPT, 33, 7, 2, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet 2770

L_056E:
    VMReturn

L_0570:
    // "Construction Truck Quiz!\nFor short: TruQ![f000]븁\u0000\nNow, here's the question![f000]븁\u0000\nWhich question is this?"
    ActorMsg MSGFILE_SCRIPT, 17, 7, 2, 0
    MsgWaitAdvance
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32801
    ListMenuAdd 18, 65535, 0
    ListMenuAdd 19, 65535, 1
    ListMenuAdd 20, 65535, 2
    ListMenuAdd 21, 65535, 3
    ListMenuShow
    VMStackPush 0x8021
    VMStackPushConst 2
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_05DC
    // "...[f000]븁\u0000\nCorrect!\nWell done![f000]븁\u0000\nYou're sharp!"
    ActorMsg MSGFILE_SCRIPT, 32, 7, 2, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet 2769
    WorkSetConst 0x40fe, 4
    VMJump L_05F0

L_05DC:
    // "...[f000]븁\u0000\nHmm... Close!\nToo bad![f000]븁\u0000\nBut you were on the right track!"
    ActorMsg MSGFILE_SCRIPT, 33, 7, 2, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet 2770

L_05F0:
    VMReturn

L_05F2:
    // "Construction Truck Quiz!\nFor short: TruQ![f000]븁\u0000\nNow, here's the question![f000]븁\u0000\nWhich Gym has a yellow drill car?"
    ActorMsg MSGFILE_SCRIPT, 22, 7, 2, 0
    MsgWaitAdvance
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32801
    ListMenuAdd 23, 65535, 0
    ListMenuAdd 24, 65535, 1
    ListMenuAdd 25, 65535, 2
    ListMenuAdd 26, 65535, 3
    ListMenuShow
    VMStackPush 0x8021
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_065E
    // "...[f000]븁\u0000\nCorrect!\nWell done![f000]븁\u0000\nYou're sharp!"
    ActorMsg MSGFILE_SCRIPT, 32, 7, 2, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet 2769
    WorkSetConst 0x40fe, 5
    VMJump L_0672

L_065E:
    // "...[f000]븁\u0000\nHmm... Close!\nToo bad![f000]븁\u0000\nBut you were on the right track!"
    ActorMsg MSGFILE_SCRIPT, 33, 7, 2, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet 2770

L_0672:
    VMReturn

L_0674:
    // "Construction Truck Quiz!\nFor short: TruQ![f000]븁\u0000\nNow, here's the question![f000]븁\u0000\nHow many bulldozers\nare there in Twist Mountain?"
    ActorMsg MSGFILE_SCRIPT, 27, 7, 2, 0
    MsgWaitAdvance
    ListMenu_AnchorTopRight 31, 5, 0, 1, 32801
    ListMenuAdd 28, 65535, 0
    ListMenuAdd 29, 65535, 1
    ListMenuAdd 30, 65535, 2
    ListMenuAdd 31, 65535, 3
    ListMenuShow
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0700
    // "...[f000]븁\u0000\nCorrect!\nWell done![f000]븁\u0000\nYou're sharp!"
    ActorMsg MSGFILE_SCRIPT, 32, 7, 2, 0
    MsgWaitAdvance
    // "Wow! You're amazing!\nYou got a perfect score on the TruQ![f000]븁\u0000\nI thought of those questions with\nall my might, you know![f000]븁\u0000\nYou are a true Heavy Machinery Pro...[f000]븁\u0000\nYes! You're a Heavy Machinery Pro\nrecognized by a wandering judge[f000]븀\u0000\nfrom the Medal Office, which is me!"
    ActorMsg MSGFILE_SCRIPT, 36, 7, 2, 0
    MsgWaitAdvance
    // "I'll tell Mr. Medal about your talent\nas a Heavy Machinery Pro."
    ActorMsg MSGFILE_SCRIPT, 37, 7, 2, 0
    LastKeyWait
    MsgWinCloseAll
    MedalGive 99
    FlagSet 2769
    WorkSetConst 0x40fe, 6
    VMJump L_0714

L_0700:
    // "...[f000]븁\u0000\nHmm... Close!\nToo bad![f000]븁\u0000\nBut you were on the right track!"
    ActorMsg MSGFILE_SCRIPT, 33, 7, 2, 0
    LastKeyWait
    MsgWinCloseAll
    FlagSet 2770

L_0714:
    VMReturn
    .balign 4, 0
