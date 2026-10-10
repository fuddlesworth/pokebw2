#include "asm/field_script.inc"
#include "text/script/clay_tunnel_3.h"

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
    // "Oh, Trainer!\nWant to ride this mining cart?"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhTrainerWantRide, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0085
    // "Oh!\nLet's go by mining cart![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhLetsGoBy, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    ActorCmdExec 8, Movement_037C
    FadeWait
    ActorCmdWait
    SEPlay SEQ_SE_SW_YACONROAD_01
    SEWait
    FlagSet 956
    FlagReset 950
    RTReserveScript 7
    MapChangeCore ZONE_CLAY_TUNNEL, 16, 0, 7, 1
    VMJump L_0093

L_0085:
    // "Oh! Anytime you'd like to ride it,\ntalk to me!"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhAnytimeYoudLike, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_0093:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_2:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, Trainer!\nWant to ride this mining cart?"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhTrainerWantRide, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0100
    // "Oh!\nLet's go by mining cart![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhLetsGoBy, 0, 0
    MsgWinCloseAll
    FadeOutBlack
    ActorCmdExec 13, Movement_037C
    FadeWait
    ActorCmdWait
    SEPlay SEQ_SE_SW_YACONROAD_01
    SEWait
    FlagSet 957
    FlagReset 949
    RTReserveScript 7
    MapChangeCore ZONE_CLAY_TUNNEL, 24, 0, 14, 2
    VMJump L_010E

L_0100:
    // "Oh! Anytime you'd like to ride it,\ntalk to me!"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhAnytimeYoudLike, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_010E:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_3:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, Trainer!\nWant to ride this mining cart?"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhTrainerWantRide, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_0197
    // "Oh!\nLet's go by mining cart![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhLetsGoBy, 0, 0
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    ActorCmdExec 9, Movement_0394
    FadeExWait
    ActorCmdWait
    SEPlay SEQ_SE_SW_YACONROAD_01
    SEWait
    FlagSet 952
    FlagReset 953
    ActorDelete 9
    ActorAdd 12
    ActorSetGPos 255, 26, 0, 73, 2
    VMSleep 60
    FadeEx 3, 16, 0, 2
    FadeExWait
    VMJump L_01A5

L_0197:
    // "Oh! Anytime you'd like to ride it,\ntalk to me!"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhAnytimeYoudLike, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_01A5:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_4:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, Trainer!\nWant to ride this mining cart?"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhTrainerWantRide, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_022E
    // "Oh!\nLet's go by mining cart![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhLetsGoBy, 0, 0
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    ActorCmdExec 12, Movement_0394
    FadeExWait
    ActorCmdWait
    SEPlay SEQ_SE_SW_YACONROAD_01
    SEWait
    FlagSet 953
    FlagReset 952
    ActorDelete 12
    ActorAdd 9
    ActorSetGPos 255, 58, 0, 90, 2
    VMSleep 60
    FadeEx 3, 16, 0, 2
    FadeExWait
    VMJump L_023C

L_022E:
    // "Oh! Anytime you'd like to ride it,\ntalk to me!"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhAnytimeYoudLike, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_023C:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_5:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, Trainer!\nWant to ride this mining cart?"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhTrainerWantRide, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_02C5
    // "Oh!\nLet's go by mining cart![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhLetsGoBy, 0, 0
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    ActorCmdExec 10, Movement_037C
    FadeExWait
    ActorCmdWait
    SEPlay SEQ_SE_SW_YACONROAD_01
    SEWait
    FlagSet 954
    FlagReset 955
    ActorDelete 10
    ActorAdd 11
    ActorSetGPos 255, 26, 0, 48, 2
    VMSleep 60
    FadeEx 3, 16, 0, 2
    FadeExWait
    VMJump L_02D3

L_02C5:
    // "Oh! Anytime you'd like to ride it,\ntalk to me!"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhAnytimeYoudLike, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_02D3:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_6:
    ActorsPauseAll
    SEPlay SEQ_SE_MESSAGE
    ActorSetEyeToEye
    // "Oh, Trainer!\nWant to ride this mining cart?"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhTrainerWantRide, 0, 0
    YesNoWin 0x8010
    VMStackPush 0x8010
    VMStackPushConst 0
    VMStackCmp CMP_EQ
    VMJumpIf CMP_STACK, L_035C
    // "Oh!\nLet's go by mining cart![f000]븁\u0000"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhLetsGoBy, 0, 0
    MsgWinCloseAll
    FadeEx 3, 0, 16, 2
    ActorCmdExec 11, Movement_0394
    FadeExWait
    ActorCmdWait
    SEPlay SEQ_SE_SW_YACONROAD_01
    SEWait
    FlagSet 955
    FlagReset 954
    ActorDelete 11
    ActorAdd 10
    ActorSetGPos 255, 47, 0, 88, 1
    VMSleep 60
    FadeEx 3, 16, 0, 2
    FadeExWait
    VMJump L_036A

L_035C:
    // "Oh! Anytime you'd like to ride it,\ntalk to me!"
    ParentActorMsg MSGFILE_SCRIPT, ClayTunnel3_Text_OhAnytimeYoudLike, 0, 0
    LastKeyWait
    MsgWinCloseAll

L_036A:
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Script_7:
    ActorsPauseAll
    FadeInBlackQ
    FadeWait
    FinishAllEvents
    ActorsUnpauseAll
    VMHalt

Movement_037C:
    Move 32, 1
    MoveEnd
    Move 33, 1
    MoveEnd
    Move 34, 1
    MoveEnd

Movement_0394:
    Move 35, 1
    MoveEnd
