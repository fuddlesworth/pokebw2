#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntry Script_2
    ScriptEntry Script_3
    ScriptEntriesEnd
    WorkSetConst 0x8020, 0

Script_1:
    FlagGet 367, 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_003B
    BMSetVisible 8, 31, 50, 0
    RTReserveScript 2

L_003B:
    VMHalt

Script_2:
    ActorsPauseAll
    FlagReset 367
    BMSetVisible 8, 31, 50, 1
    BMCreateHandleByGPos 0x8020, 8, 31, 50
    BMHndAudioVisualAnmPlay 0x8020, 0
    BMHndAnmWait 0x8020
    BMReleaseHandle 0x8020
    SEPlay SEQ_SE_FLD_118
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    FlagGet 2407, 0x8021
    FlagGet 2408, 0x8022
    FlagGet 2409, 0x8023
    FlagGet 2410, 0x8024
    VMStackPush 0x8021
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8022
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_00EC
    CallLeagueLiftWarp
    VMJump L_00F9

L_00EC:
    SEPlay SEQ_SE_MESSAGE
    // "Words are engraved on the statue:[f000]븁\u0000\n“Four great warriors form\n this Pokémon League.[f000]븁\u0000\n To the southwest is one who\n does not fear the Ghost type.[f000]븁\u0000\n To the southeast is one who\n channels the power of the Fighting type.[f000]븁\u0000\n To the northwest is one who\n has mastered the Dark type.[f000]븁\u0000\n To the northeast is one who\n knows the mind of the Psychic type.[f000]븁\u0000\n If you can defeat these warriors with\n your courage and wisdom,[f000]븀\u0000\n you shall be led to the summit,[f000]븀\u0000\n where the strongest Champion awaits.\""
    InfoMsg 0, 2
    LastKeyWait
    MsgWinCloseAll

L_00F9:
    WorkSetConst 0x8024, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8021, 0
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
