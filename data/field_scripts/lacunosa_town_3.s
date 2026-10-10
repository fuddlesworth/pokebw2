#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0

Script_1:
    RTCGetDayPart 0x8023
    VMStackPush 0x8023
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0063
    FlagReset 786
    FlagReset 783
    VMJump L_006B

L_0063:
    FlagSet 786
    FlagSet 783

L_006B:
    VMStackPush 0x40cc
    VMStackPushConst 4
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0086
    DebugPrint 2323
    FlagReset 783

L_0086:
    VMHalt

Script_2:
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x40cc, 2
    ActorWalkRoute 255, 6, 5, 1, 8, 1
    ActorCmdWait
    FlagReset 782
    ActorAdd 2
    SEPlay SEQ_SE_KAIDAN
    SEWait
    ActorWalkRoute 2, 6, 6, 1, 8, 1
    ActorCmdWait
    ActorWalkRoute 2, 7, 5, 1, 8, 0
    ActorCmdWait
    RTCGetDayPart 0x8023
    VMStackPush 0x8023
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0113
    // "You must be the ones who want to hear\nthat old tale about Lacunosa Town.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 0, 0, 0, 0
    MsgWinCloseAll
    // "Professor Juniper: That's right.\nPlease tell us.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 1, 1, 0, 0
    MsgWinCloseAll

L_0113:
    // "Behind Lacunosa Town,\nthere's a mighty big hole.[f000]븁\u0000\nHave you heard of the Giant Chasm?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 2, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0320
    VMSleep 3
    ActorCmdExec 255, Movement_0318
    ActorCmdWait
    // "Bianca: Oh, I've heard that around the\nGiant Chasm, there have been brief[f000]븀\u0000\ntemperature readings of -58° F![f000]븁\u0000\nThat's what Cheren told me, anyway![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 3, 2, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0318
    ActorCmdWait
    // "Professor Juniper: The road is blocked,\nso we can't get there right now...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 4, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0328
    ActorCmdExec 2, Movement_0328
    VMSleep 3
    ActorCmdExec 255, Movement_0328
    ActorCmdWait
    // "A long, long time ago, the\nGiant Chasm was created when[f000]븀\u0000\na big meteorite fell from the sky.[f000]븁\u0000\nA really scary Pokémon was\nhidden inside that meteorite...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 5, 0, 0, 0
    MsgWinCloseAll
    // "Professor Juniper: A meteorite...[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 6, 1, 0, 0
    MsgWinCloseAll
    // "When darkness falls over the land,\nthis Pokémon appears.[f000]븀\u0000\nA frigid wind follows it.[f000]븁\u0000\nIt freezes everything around\nand eats people and Pokémon...[f000]븁\u0000\nThat's why everyone was afraid.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 7, 0, 0, 0
    MsgWinCloseAll
    ActorCmdExec 2, Movement_0338
    ActorCmdWait
    // "Bianca: The Pokémon ate p-people?[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 8, 2, 0, 0
    MsgWinCloseAll
    // "So our ancestors surrounded the town\nwith walls, to prevent the Pokémon[f000]븀\u0000\nfrom getting inside the town.[f000]븁\u0000\nAlso, a rule was made forbidding\nanyone to go outside after dark.[f000]븁\u0000\n...And that's the end of the old tale![f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 9, 0, 0, 0
    MsgWinCloseAll
    // "Professor Juniper: A fascinating story!\nI'll add it to my research records.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 10, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_0318
    VMSleep 8
    ActorCmdExec 2, Movement_0350
    ActorCmdExec 255, Movement_0350
    ActorCmdWait
    // "Everyone, we should be going.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 11, 1, 0, 0
    MsgWinCloseAll
    ActorCmdExec 1, Movement_02EC
    ActorCmdExec 2, Movement_0300
    VMSleep 16
    ActorCmdExec 255, Movement_0330
    ActorCmdWait
    SEPlay SEQ_SE_KAIDAN
    ActorDelete 1
    ActorDelete 2
    SEWait
    FlagSet 781
    FlagSet 782
    FlagReset 784
    WorkSetConst 0x40ce, 1
    HollowRivalCmd_0262 3, 6
    HollowRivalCmd_0262 0, 5
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPush 0x40cc
    VMStackPushConst 4
    VMStackCmp CMP_NE
    VMJumpIf CMP_STACK, L_0284
    // "That's sure a scary-sounding Pokémon,\neven in an old folktale."
    ParentActorMsg MSGFILE_SCRIPT, 12, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02C7

L_0284:
    // "Zzz... Zzz...[f000]븁\u0000\n...Wh-what?\nWhat do you want, now?[f000]븁\u0000\nDid you pick now to listen to my stories?"
    ParentActorMsg MSGFILE_SCRIPT, 13, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02B9
    // "Mmm... OK, I'll tell you...[f000]븁\u0000\nBehind our Lacunosa Town,\nthere's a big hole in the ground.[f000]븁\u0000\nThat hole, way in the past...[f000]븁\u0000\nAaaahhh, I'm so tired...[f000]븁\u0000\nThat hole, there was a...a big...\ncrashed down...inside was...[f000]븁\u0000\nbig, scary...really scary...[f000]븁\u0000\nZzz... Zzz... Zzz..."
    ParentActorMsg MSGFILE_SCRIPT, 14, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_02C7

L_02B9:
    // "Aaaahhh... Oh, I should...\nOK, I'll sleep, then...[f000]븁\u0000\nGoodni... Zzz...\nZzz... Zzz..."
    ParentActorMsg MSGFILE_SCRIPT, 15, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02C7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My grandma loves old stories![f000]븁\u0000\nI'm always having to listen\nto her really long stories.[f000]븁\u0000\nBut sometimes if it's night, she'll\nfall asleep right in the middle of a story.[f000]븁\u0000\nIt's OK, though. She's not only healthy,\nshe's a free-spirited grandma, too!"
    ParentActorMsg MSGFILE_SCRIPT, 16, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_02EC:
    Move 13, 2
    Move 15, 1
    Move 13, 2
    Move 69, 0
    MoveEnd

Movement_0300:
    Move 63, 1
    Move 13, 2
    Move 14, 1
    Move 13, 2
    Move 69, 0
    MoveEnd

Movement_0318:
    Move 35, 1
    MoveEnd

Movement_0320:
    Move 34, 1
    MoveEnd

Movement_0328:
    Move 32, 1
    MoveEnd

Movement_0330:
    Move 33, 1
    MoveEnd

Movement_0338:
    Move 75, 1
    MoveEnd
    Move 0, 1
    MoveEnd
    Move 1, 1
    MoveEnd

Movement_0350:
    Move 2, 1
    MoveEnd
    Move 3, 1
    MoveEnd
