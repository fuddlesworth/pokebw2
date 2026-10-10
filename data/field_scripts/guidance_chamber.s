#include "asm/field_script.inc"

    ScriptEntry Script_1
    ScriptEntriesEnd

Script_1:
    ActorsPauseAll
    WorkSetConst 0x8020, 0
    WorkSetConst 0x8021, 0
    WorkSetConst 0x8022, 0
    WorkSetConst 0x8023, 0
    WorkSetConst 0x8024, 0
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "I've been searching for the legendary\nPokémon Cobalion for decades..."
    ParentActorMsg MSGFILE_SCRIPT, 0, 0, 0
    MsgWaitAdvance
    PokePartyGetCount 0x8020, 0

L_003E:
    VMStackPush 0x8020
    VMStackPush 0x8021
    VMStackCmp CMP_GT
    VMJumpIf CMP_STACK, L_0092
    PokePartyGetSpecies 0x8022, 0x8021
    PokePartyIsEgg 0x8024, 0x8021
    VMStackPush 0x8022
    VMStackPushConst 638
    VMStackCmp CMP_EQ
    VMStackPush 0x8024
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMStackCmp CMP_AND
    VMJumpIf CMP_STACK, L_0086
    WorkSetConst 0x8023, 1

L_0086:
    WorkAdd 0x8021, 1
    VMJump L_003E

L_0092:
    VMStackPush 0x8023
    VMStackPushConst 1
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_00C9
    MsgWinCloseAll
    ActorCmdExec 2, Movement_00E0
    ActorCmdWait
    VMSleep 8
    // "Oh! It's...\nIt's Cobalion![f000]븁\u0000\nNow I understand.[f000]븁\u0000\nYou were able to show it that\nthere are humans and Pokémon[f000]븀\u0000\nthat understand one another[f000]븀\u0000\nand help each other out![f000]븁\u0000\nBut it's not just humans and Pokémon...\nAll living things must accept[f000]븀\u0000\nand trust each other.[f000]븁\u0000\nThat's the best way to look at it."
    ParentActorMsg MSGFILE_SCRIPT, 1, 0, 0
    LastKeyWait
    MsgWinCloseAll
    VMJump L_00D7

L_00C9:
    // "I wonder if it still hates humans.[f000]븁\u0000\nOr...maybe it looks at this world\nwhere Pokémon and people coexist[f000]븀\u0000\nand has thoughts about it..."
    ParentActorMsg MSGFILE_SCRIPT, 2, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_00D7:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt
    .balign 4, 0

Movement_00E0:
    Move 75, 1
    MoveEnd
