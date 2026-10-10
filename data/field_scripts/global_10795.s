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
    ScriptEntry Script_13
    ScriptEntry Script_14
    ScriptEntry Script_15
    ScriptEntry Script_16
    ScriptEntry Script_17
    ScriptEntry Script_18
    ScriptEntry Script_19
    ScriptEntry Script_20
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

L_0058:
    SEPlay SEQ_SE_MESSAGE
    // "Look!\nYou've found a narrow path![f000]븁\u0000\nWill you follow it?"
    SystemMsg 0, 2
    YesNoWin 0x8010
    InfoMsgClose
    SEWait
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0089
    Cmd_02C5 27
    SEPlay SEQ_SE_KAIDAN
    HiddenHollowCallWarpIn 0x8020

L_0089:
    VMReturn

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    VMCall L_0058
    FlagSet 2640
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    WorkSetConst 0x8020, 1
    VMCall L_0058
    FlagSet 2641
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8020, 2
    VMCall L_0058
    FlagSet 2642
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    WorkSetConst 0x8020, 3
    VMCall L_0058
    FlagSet 2643
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    WorkSetConst 0x8020, 4
    VMCall L_0058
    FlagSet 2644
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    WorkSetConst 0x8020, 5
    VMCall L_0058
    FlagSet 2645
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    WorkSetConst 0x8020, 6
    VMCall L_0058
    FlagSet 2646
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_8:
    ActorsPauseAll
    WorkSetConst 0x8020, 7
    VMCall L_0058
    FlagSet 2647
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_9:
    ActorsPauseAll
    WorkSetConst 0x8020, 8
    VMCall L_0058
    FlagSet 2648
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_10:
    ActorsPauseAll
    WorkSetConst 0x8020, 9
    VMCall L_0058
    FlagSet 2649
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_11:
    ActorsPauseAll
    WorkSetConst 0x8020, 10
    VMCall L_0058
    FlagSet 2650
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_12:
    ActorsPauseAll
    WorkSetConst 0x8020, 11
    VMCall L_0058
    FlagSet 2651
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_13:
    ActorsPauseAll
    WorkSetConst 0x8020, 12
    VMCall L_0058
    FlagSet 2652
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_14:
    ActorsPauseAll
    WorkSetConst 0x8020, 13
    VMCall L_0058
    FlagSet 2653
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_15:
    ActorsPauseAll
    WorkSetConst 0x8020, 14
    VMCall L_0058
    FlagSet 2654
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_16:
    ActorsPauseAll
    WorkSetConst 0x8020, 15
    VMCall L_0058
    FlagSet 2655
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_17:
    ActorsPauseAll
    WorkSetConst 0x8020, 16
    VMCall L_0058
    FlagSet 2656
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_18:
    ActorsPauseAll
    WorkSetConst 0x8020, 17
    VMCall L_0058
    FlagSet 2657
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_19:
    ActorsPauseAll
    WorkSetConst 0x8020, 18
    VMCall L_0058
    FlagSet 2658
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_20:
    ActorsPauseAll
    WorkSetConst 0x8020, 19
    VMCall L_0058
    FlagSet 2659
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
