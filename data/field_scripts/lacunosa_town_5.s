#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    RTCGetDayPart 0x8020
    VMStackPush 0x8020
    VMStackPushConst 3
    VMStackCmp CMP_EQ
    VMStackPush 0x8020
    VMStackPushConst 4
    VMStackCmp CMP_EQ
    VMStackCmp CMP_OR
    VMJumpIf CMP_STACK, L_0045
    FlagReset 786
    VMJump L_0049

L_0045:
    FlagSet 786

L_0049:
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "There are a lot of people in the world,\nand there are just as many different[f000]븀\u0000\ncharacteristics and ideas.[f000]븁\u0000\nI think I'd be really happy if I could\nmeet a lot of people and see the[f000]븀\u0000\ndifferences for myself!"
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    VMStackPushFlag 2784
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0142
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "My work always keeps me very busy, so\nI'm thrilled if I can go home at night.[f000]븁\u0000\nI'm sorry to leave my wife lonely, but\nthat's the life of a powerful executive.[f000]븁\u0000\nOh, this is a souvenir from a business\ntrip. She does not seem to need it,[f000]븀\u0000\nso I will give it to you.[f000]븁\u0000"
    ActorMsg MSGFILE_SCRIPT, 1, 2, 0, 0
    ActorMsgClose
    Random 0x400f, 3
    VMStackPush 0x400f
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00CF
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 151
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_0128

L_00CF:
    VMStackPush 0x400f
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0108
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 165
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000
    VMJump L_0128

L_0108:
    VMStackPush 0x8000
    VMStackPush 0x8001
    WorkSet 0x8000, 154
    WorkSet 0x8001, 1
    RTCallGlobal 2805
    VMStackPop 0x8001
    VMStackPop 0x8000

L_0128:
    // "Tomorrow it's work as always![f000]븁\u0000\nI'm a super businessman, aren't I?"
    ActorMsg MSGFILE_SCRIPT, 2, 2, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet 2784
    VMJump L_0156

L_0142:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Tomorrow it's work as always![f000]븁\u0000\nI'm a super businessman, aren't I?"
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0
    LastKeyWait
    ActorMsgClose

L_0156:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
