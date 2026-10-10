#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x4165
    VMStackPushConst 0
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_008D
    VMStackPushFlag 239
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0054
    // "Oh ho! With a face like that,\nI'll bet you're a Pokémon Trainer![f000]븁\u0000\nI work as a Depot Agent on the\nBattle Subway![f000]븁\u0000\nHave you tried challenging the\nBattle Subway yet?[f000]븁\u0000\nIt's a hot spot for those who want\nsome serious battling action![f000]븁\u0000\nBut some people get too wrapped up\nin their battles, and there's no end[f000]븀\u0000\nto the lost-and-found items.[f000]븁\u0000\nThat's right. Nobody ever came to\npick this up, so I'll give it to you![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    FlagSet 239
    WorkSetConst 0x4164, 50
    VMCall L_00A1
    VMJump L_0087

L_0054:
    VMStackPush 0x4165
    VMStackPushConst 5
    VMStackCmp CMP_LT
    VMJumpIf CMP_STACK, L_0077
    // "I have a lost-and-found item\nthat no one has claimed.[f000]븁\u0000\nIf you don't mind, could you take it\noff my hands?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, 1, 0, 0
    VMJump L_0081

L_0077:
    // "Oh! I've been waiting for you![f000]븁\u0000\nRecently, we've gotten more passengers,\nso there's more lost-and-found items![f000]븁\u0000\nNo one has claimed this, so if you don't\nmind, could you take it off my hands?[f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0

L_0081:
    VMCall L_00A1

L_0087:
    VMJump L_009B

L_008D:
    // "If I have another lost-and-found\nitem, come get it, OK?[f000]븁\u0000\nAlso, if you're interested, take on the\nBattle Subway in Nimbasa City!"
    ParentActorMsg MSGFILE_SCRIPT, 4, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_009B:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

L_00A1:
    ItemCheckSpace 0x4164, 0x4165, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00D1
    WordSetItemName 0, 0x4164
    // "Oh, my. There's no more room for\nthe [f000]ĉ\u0001\u0000.[f000]븁\u0000\nYou'll need to remove some from your Bag\nbefore you can accept any more!"
    ParentActorMsg MSGFILE_SCRIPT, 5, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMReturn

L_00D1:
    MsgWinCloseAll
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 0x4164
    WorkSet 0x8001, 0x4165
    RTCallGlobal 2801
    VMStackPop 0x8001
    VMStackPop 0x8000
    // "No worries. No worries.\nJust take it![f000]븁\u0000\nIt's better for a Trainer to use it than\nfor me to hold on to it.[f000]븁\u0000\nI'm sure the item is happier, too!"
    ParentActorMsg MSGFILE_SCRIPT, 3, 0, 0
    LastKeyWait
    MsgWinCloseAll
    WorkSetConst 0x4165, 0
    Random 0x8010, 10
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_0120
    VMJump L_012C

L_0120:
    WorkSetConst 0x4164, 50
    VMJump L_022A

L_012C:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_013F
    VMJump L_014B

L_013F:
    WorkSetConst 0x4164, 23
    VMJump L_022A

L_014B:
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_015E
    VMJump L_016A

L_015E:
    WorkSetConst 0x4164, 29
    VMJump L_022A

L_016A:
    WorkCmpConst 0x8010, 3
    VMJumpIf CMP_EQ, L_017D
    VMJump L_0189

L_017D:
    WorkSetConst 0x4164, 40
    VMJump L_022A

L_0189:
    WorkCmpConst 0x8010, 4
    VMJumpIf CMP_EQ, L_019C
    VMJump L_01A8

L_019C:
    WorkSetConst 0x4164, 46
    VMJump L_022A

L_01A8:
    WorkCmpConst 0x8010, 5
    VMJumpIf CMP_EQ, L_01BB
    VMJump L_01C7

L_01BB:
    WorkSetConst 0x4164, 47
    VMJump L_022A

L_01C7:
    WorkCmpConst 0x8010, 6
    VMJumpIf CMP_EQ, L_01DA
    VMJump L_01E6

L_01DA:
    WorkSetConst 0x4164, 49
    VMJump L_022A

L_01E6:
    WorkCmpConst 0x8010, 7
    VMJumpIf CMP_EQ, L_01F9
    VMJump L_0205

L_01F9:
    WorkSetConst 0x4164, 52
    VMJump L_022A

L_0205:
    WorkCmpConst 0x8010, 8
    VMJumpIf CMP_EQ, L_0218
    VMJump L_0224

L_0218:
    WorkSetConst 0x4164, 48
    VMJump L_022A

L_0224:
    WorkSetConst 0x4164, 45

L_022A:
    VMReturn
