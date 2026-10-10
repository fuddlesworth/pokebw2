#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    PVPlay 637, 0
    // "Vraahhbrbrbr!"
    ScreamMsg 0, 2
    PVWait
    MsgWaitAdvance
    InfoMsgClose_0039
    VMStackPushFlag 404
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_003E
    CallWildBattle 637, 35, 1
    VMJump L_0046

L_003E:
    CallWildBattle 637, 65, 1

L_0046:
    WildBattleIsVictory 0x8010
    VMStackPush 0x8010
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0071
    FlagSet 928
    FlagSet 404
    ActorDelete 0
    CallWildBattleEnd
    VMJump L_0073

L_0071:
    CallWildLose

L_0073:
    WildBattleGetResult 0x8010
    WorkCmpConst 0x8010, 0
    VMJumpIf CMP_EQ, L_008A
    VMJump L_0094

L_008A:
    FlagSet 403
    VMJump L_00C4

L_0094:
    WorkCmpConst 0x8010, 1
    VMJumpIf CMP_EQ, L_00B4
    WorkCmpConst 0x8010, 2
    VMJumpIf CMP_EQ, L_00B4
    VMJump L_00C4

L_00B4:
    // "Volcarona quietly flew away..."
    SystemMsg 1, 2
    LastKeyWait
    InfoMsgClose
    VMJump L_00C4

L_00C4:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0
