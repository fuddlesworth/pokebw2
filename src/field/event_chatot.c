#include "types.h"
#include "constants/arc.h"
#include "constants/species.h"
#include "field/event_chatot.h"
#include "field/event_sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_msgbg.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "save/chatter.h"
#include "struct_decls.h"
#include "system/app_keycursor.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"

// The sprite of the Chatot
static const ClActorSetup sSpriteSetup = { 128, 72, 0, 0, 0 };

GameEvent *EventChatotRecord_Create(GameSystem *gsys, Field *field, u8 partyIndex) {
    GameEvent *event;
    ChatotEventWork *work;
    PokeParty *party;

    event = GameEvent_Create(gsys, NULL, EventChatotRecord_Main, sizeof(ChatotEventWork));
    work = GameEvent_GetData(event);
    MI_CpuFill8(work, 0, sizeof(ChatotEventWork));
    work->gsys = gsys;
    work->gameData = GSYS_GetGameData(gsys);
    work->chatter = getChatterBlockAddress(GameData_GetSaveControl(work->gameData));
    work->field = field;
    work->player = Field_GetPlayer(field);
    party = GameData_GetParty(work->gameData);
    work->pkm = PokeParty_GetPkm(party, partyIndex);
    work->msgBGSys = Field_GetMsgBGSys(work->field);
    work->partyIndex = partyIndex;
    work->recorded = 0;
    return event;
}

// The overwrite prompt, microphone recording, playback and cleanup.
enum {
    CHATOT_RECORD_STEP_PAUSE_ACTORS = 0,
    CHATOT_RECORD_STEP_OPEN_PROMPT = 1,
    CHATOT_RECORD_STEP_WAIT_OVERWRITE_PROMPT = 2,
    CHATOT_RECORD_STEP_CHECK_OVERWRITE_ANSWER = 3,
    CHATOT_RECORD_STEP_WAIT_RECORD_PROMPT = 4,
    CHATOT_RECORD_STEP_WAIT_MIC_READY = 5,
    CHATOT_RECORD_STEP_START_RECORDING = 6,
    CHATOT_RECORD_STEP_WAIT_RECORDING = 7,
    CHATOT_RECORD_STEP_ANIMATE_ICON = 8,
    CHATOT_RECORD_STEP_WAIT_RECORDED_MESSAGE = 9,
    CHATOT_RECORD_STEP_PLAY_RECORDING = 10,
    CHATOT_RECORD_STEP_WAIT_PLAYBACK = 11,
    CHATOT_RECORD_STEP_WAIT_ERROR_MESSAGE = 12,
    CHATOT_RECORD_STEP_CLEANUP = 13,
    CHATOT_RECORD_STEP_RESUME_ACTORS = 14,
};

GameEventReturnCode EventChatotRecord_Main(GameEvent *event, u32 *state, void *data) {
    ChatotEventWork *work = data;
    StrBuf *message;
    PokeVoiceChatterInfo chatterInfo;

    switch (*state) {
    case CHATOT_RECORD_STEP_PAUSE_ACTORS:
        EventChatotRecord_PauseActors(work);
        *state = CHATOT_RECORD_STEP_OPEN_PROMPT;
        break;
    case CHATOT_RECORD_STEP_OPEN_PROMPT:
        work->msgData = MsgBGPrinter_CreateMsgData(work->msgBGSys, 0x6c);
        work->talkWindow = MsgBGPrinter_CreateBottomSysWin(work->msgBGSys, work->msgData);
        work->wordSet = GFL_WordSetSystemCreateDefault(21);
        work->strbuf = GFL_StrBufCreate(0x400, 21);
        EventChatotRecord_SetupWindow(work);
        if (doesChatotExist(work->chatter) == TRUE) {
            MsgBGPrinter_SysWinPrintMsg(work->talkWindow, 0, 0, 0);
            *state = CHATOT_RECORD_STEP_WAIT_OVERWRITE_PROMPT;
        } else {
            MsgBGPrinter_SysWinPrintMsg(work->talkWindow, 0, 0, 6);
            *state = CHATOT_RECORD_STEP_WAIT_RECORD_PROMPT;
        }
        break;
    case CHATOT_RECORD_STEP_WAIT_OVERWRITE_PROMPT:
        if (MsgBGPrinter_SysWinMain(work->talkWindow) == TRUE) {
            work->yesNo = MsgBGPrinter_CreateYesNoMenu(work->msgBGSys, 0);
            *state = CHATOT_RECORD_STEP_CHECK_OVERWRITE_ANSWER;
        }
        break;
    case CHATOT_RECORD_STEP_CHECK_OVERWRITE_ANSWER:
        switch (MsgBGPrinter_YesNoMenuMain(work->yesNo)) {
        case MSGBG_YES_NO_YES:
            MsgBGPrinter_DeleteMenuFunc(work->yesNo);
            MsgBGPrinter_ClearSysWin(work->talkWindow);
            MsgBGPrinter_SysWinPrintMsg(work->talkWindow, 0, 0, 6);
            *state = CHATOT_RECORD_STEP_WAIT_RECORD_PROMPT;
            break;
        case MSGBG_YES_NO_NO:
            MsgBGPrinter_DeleteMenuFunc(work->yesNo);
            *state = CHATOT_RECORD_STEP_CLEANUP;
            break;
        }
        break;
    case CHATOT_RECORD_STEP_WAIT_RECORD_PROMPT:
        if (MsgBGPrinter_SysWinMain(work->talkWindow) == TRUE) {
            GCTX_HIDBlockSleep(8);
            setupMic(21);
            work->waitIcon =
                WaitIcon_Create(GFL_VBlankGetTCBMgr(), MsgBGPrinter_GetSysWinBmpWin(work->talkWindow), 15, 16, 21);
            *state = CHATOT_RECORD_STEP_WAIT_MIC_READY;
        }
        break;
    case CHATOT_RECORD_STEP_WAIT_MIC_READY:
        SndMic_AddCount(2);
        if (SndMic_IsOverLimit() == TRUE) {
            WaitIcon_Free(work->waitIcon);
            MsgBGPrinter_ClearSysWin(work->talkWindow);
            MsgBGPrinter_SysWinPrintMsg(work->talkWindow, 0, 0, 1);
            GameEvent_ChainNext(event, EventBGMPushWait_Create(work->gsys, 6));
            *state = CHATOT_RECORD_STEP_START_RECORDING;
        }
        break;
    case CHATOT_RECORD_STEP_START_RECORDING:
        if (MsgBGPrinter_SysWinMain(work->talkWindow) == TRUE) {
            if (SndMic_StartRecording(EventChatotRecord_OnRecorded, &work->recorded) == 0) {
                *state = CHATOT_RECORD_STEP_WAIT_RECORDING;
            } else {
                message = GFL_MsgDataLoadStrbufNew(work->msgData, 3);
                loadPokemonNicknameToStrbuf(work->wordSet, 0, work->pkm);
                GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, message);
                GFL_StrBufFree(message);
                MsgBGPrinter_ClearSysWin(work->talkWindow);
                MsgBGPrinter_SysWinPrintStr(work->talkWindow, 0, 0, work->strbuf);
                ampOffFreeBlocks();
                GCTX_HIDUnblockSleep(8);
                GameEvent_ChainNext(event, EventPushBGMFinish_Create(work->gsys, 0, 30));
                *state = CHATOT_RECORD_STEP_WAIT_ERROR_MESSAGE;
            }
        }
        break;
    case CHATOT_RECORD_STEP_WAIT_RECORDING:
        if (work->recorded == TRUE) {
            SndMic_EncodeChatter(work->chatter);
            ampOffFreeBlocks();
            GCTX_HIDUnblockSleep(8);
            GameEvent_ChainNext(event, EventPushBGMFinish_Create(work->gsys, 0, 30));
            *state = CHATOT_RECORD_STEP_ANIMATE_ICON;
        }
        break;
    case CHATOT_RECORD_STEP_ANIMATE_ICON:
        if (!EventChatotRecord_HopPokeIcon(work)) {
            message = GFL_MsgDataLoadStrbufNew(work->msgData, 2);
            loadPokemonNicknameToStrbuf(work->wordSet, 0, work->pkm);
            GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, message);
            GFL_StrBufFree(message);
            MsgBGPrinter_ClearSysWin(work->talkWindow);
            MsgBGPrinter_SysWinPrintStr(work->talkWindow, 0, 0, work->strbuf);
            *state = CHATOT_RECORD_STEP_WAIT_RECORDED_MESSAGE;
        }
        break;
    case CHATOT_RECORD_STEP_WAIT_RECORDED_MESSAGE:
        if (MsgBGPrinter_SysWinMain(work->talkWindow) == TRUE) {
            *state = CHATOT_RECORD_STEP_PLAY_RECORDING;
        }
        break;
    case CHATOT_RECORD_STEP_PLAY_RECORDING:
        PokeVoice_CreateChatterInfo(&chatterInfo);
        work->voice = PokeVoice_Play(SPECIES_CHATOT, 0, 64, 0, 0, 0, 0, &chatterInfo);
        *state = CHATOT_RECORD_STEP_WAIT_PLAYBACK;
        break;
    case CHATOT_RECORD_STEP_WAIT_PLAYBACK:
        if (!PokeVoice_IsPlaying(work->voice)) {
            *state = CHATOT_RECORD_STEP_CLEANUP;
        }
        break;
    case CHATOT_RECORD_STEP_WAIT_ERROR_MESSAGE:
        if (MsgBGPrinter_SysWinMain(work->talkWindow) == TRUE) {
            *state = CHATOT_RECORD_STEP_CLEANUP;
        }
        break;
    case CHATOT_RECORD_STEP_CLEANUP:
        EventChatotRecord_Cleanup(work);
        GFL_StrBufFree(work->strbuf);
        GFL_WordSetSystemFree(work->wordSet);
        MsgBGPrinter_DeleteSysWin(work->talkWindow);
        MsgBGPrinter_DeleteMsgData(work->msgData);
        *state = CHATOT_RECORD_STEP_RESUME_ACTORS;
        break;
    case CHATOT_RECORD_STEP_RESUME_ACTORS:
        EventChatotRecord_ResumeActors(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void EventChatotRecord_OnRecorded(u32 result, u32 *done) {
    *done = TRUE;
}

void EventChatotRecord_PauseActors(ChatotEventWork *work) {
    DisableAllActorsMovement(Field_GetActorSystem(GSYS_GetField(work->gsys)));
}

void EventChatotRecord_ResumeActors(ChatotEventWork *work) {
    EnableAllActorsMovement(Field_GetActorSystem(GSYS_GetField(work->gsys)));
}

void EventChatotRecord_CreatePokeIcon(ChatotEventWork *work) {
    BoxPkm *pkm;
    ArcTool *arc;
    BOOL encrypted;
    ClActorSetup setup;

    work->unit = ClActUnit_Create(1, 0, 21);
    pkm = PokeParty_GetBoxPkm(work->pkm);
    arc = MakePokeGraArcHandle(21);
    encrypted = PML_PkmDecrypt(pkm);
    work->chars = PokeGra_LoadClActCharsByBoxData(arc, pkm, POKEGRA_DIR_FRONT, 0, 21);
    work->palette = PokeGra_LoadClActPaletteByBoxData(arc, pkm, POKEGRA_DIR_FRONT, 0, 0xc0, 21);
    work->cellAnims = PokeGra_LoadClActCellAnimsByBoxData(pkm, POKEGRA_DIR_FRONT, 1, 0, 21);
    PML_PkmReEncrypt(pkm, encrypted);
    GFL_ArcToolFree(arc);
    setup = sSpriteSetup;
    work->sprite = ClActor_Create(work->unit, work->chars, work->palette, work->cellAnims, &setup, 0, 21);
}

void EventChatotRecord_DeletePokeIcon(ChatotEventWork *work) {
    ClActor_Delete(work->sprite);
    ClActSys_FreeChars(work->chars);
    ClActSys_FreePltt(work->palette);
    ClActSys_FreeCellAnims(work->cellAnims);
    ClActUnit_Delete(work->unit);
}

u32 EventChatotRecord_HopPokeIcon(ChatotEventWork *work) {
    ClActorPos position;

    if (work->animFrame == 0) {
        work->animOffset = -4;
    }
    if (work->animOffset == -4) {
        work->animOffset = 3;
        work->animFrame++;
        if (work->animFrame == 3) {
            return 0;
        }
    }
    ClActor_GetSurfacePos(work->sprite, &position, 0);
    position.y -= work->animOffset;
    ClActor_SetSurfacePos(work->sprite, &position, 0);
    work->animOffset--;
    return 1;
}

void EventChatotRecord_SetupWindow(ChatotEventWork *work) {
    u32 paletteId;
    GFLBitmap *bitmap;

    paletteId = GetSysMsgBoxPaletteDatID(0);
    GFL_G2DIOLoadNCLR(ARCID_WINFRAME, paletteId, 0, 0, 0, 32, 21);
    work->window = BmpWin_CreateDynamic(1, 10, 3, 12, 12, 0, 1);
    bitmap = BmpWin_GetBitmap(work->window);
    GFL_BitmapFill(bitmap, 17);
    BmpWin_FlushChar(work->window);
    BmpWin_FlushMap(work->window);
    BmpWin_DrawFrame(work->window, WINFRAME_TRANSFER_VBLANK, 1, 0);
    EventChatotRecord_CreatePokeIcon(work);
}

void EventChatotRecord_Cleanup(ChatotEventWork *work) {
    EventChatotRecord_DeletePokeIcon(work);
    BmpWin_ClearFrame(work->window, WINFRAME_TRANSFER_VBLANK);
    BmpWin_Free(work->window);
}
