#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntry Script_4
    ScriptEntry Script_5
    ScriptEntriesEnd

Script_5:
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_003F
    WorkSetConst 0x4020, 209
    WorkSetConst 0x4021, 209
    VMJump L_004B

L_003F:
    WorkSetConst 0x4020, 129
    WorkSetConst 0x4021, 129

L_004B:
    VMHalt

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    VMStackPushFlag 256
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_007E
    // "Trading Pokémon lets you get to know\nother Trainers!"
    ActorMsg MSGFILE_SCRIPT, 5, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    VMJump L_0155

L_007E:
    // "Kid! Have you caught any Emolga?[f000]븁\u0000\nIf you have, would you trade your\nEmolga for my Gigalith?"
    ActorMsg MSGFILE_SCRIPT, 0, 0, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0145
    ActorMsgClose
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    CallPokeSelect 0, 0x8021, 0x8020, 0
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_012F
    WorkSetConst 0x8022, 0
    FieldTradeCheck 0x8022, TRADE_GIGALITH, 0x8020
    VMStackPush 0x8022
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0119
    // "OK! Let's start our Pokémon trade![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, 1, 0, 0
    MsgWinCloseAll
    FieldTradeStart 26, 0x8020
    // "Oh! Oh! What a cute Pokémon!\nPlease cherish Gigalith, too."
    ActorMsg MSGFILE_SCRIPT, 2, 0, 0, 0
    LastKeyWait
    ActorMsgClose
    FlagSet 256
    VMJump L_0129

L_0119:
    // "Hey! Come on, now!\nI want to trade for an Emolga..."
    ActorMsg MSGFILE_SCRIPT, 3, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0129:
    VMJump L_013F

L_012F:
    // "I see... Well, come talk to me again if you\nchange your mind!"
    ActorMsg MSGFILE_SCRIPT, 4, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_013F:
    VMJump L_0155

L_0145:
    // "I see... Well, come talk to me again if you\nchange your mind!"
    ActorMsg MSGFILE_SCRIPT, 4, 0, 0, 0
    LastKeyWait
    ActorMsgClose

L_0155:
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
    // "Watching Pokémon play together\nmakes me really happy..."
    ParentActorMsg MSGFILE_SCRIPT, 6, 0, 0
    LastKeyWait
    ActorMsgClose
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_01C4
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 546, 0
    // "Fwoo-ooo-ooosh..."
    ParentActorMsg MSGFILE_SCRIPT, 9, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_01E0

L_01C4:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 548, 0
    // "Tralalala! ♪"
    ParentActorMsg MSGFILE_SCRIPT, 7, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose

L_01E0:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    GameGetVersion 0x8010
    VMStackPush 0x8010
    VMStackPushConst 23
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0221
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 546, 0
    // "Cotttooon. ♪"
    ParentActorMsg MSGFILE_SCRIPT, 10, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose
    VMJump L_023D

L_0221:
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    PVPlay 548, 0
    // "Peti peti!"
    ParentActorMsg MSGFILE_SCRIPT, 8, 0, 0
    PVWait
    LastKeyWait
    ActorMsgClose

L_023D:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
