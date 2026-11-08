#ifndef POKEBW2_FIELD_EVENT_CHATOT_H
#define POKEBW2_FIELD_EVENT_CHATOT_H

#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "system/app_keycursor.h"
#include "system/game_event.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"

struct ChatotEventWork {
    PerapVoice *chatter;
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    FieldPlayer *player;
    PartyPkm *pkm;
    MsgData *msgData;
    FieldMsgBG *msgBGSys;
    FieldMsgWin *talkWindow;
    ListMenuUI *yesNo;
    WordSet *wordSet;
    StrBuf *strbuf;
    BmpWin *window;
    WaitIcon *waitIcon;
    ClActUnit *unit;
    ClActor *sprite;
    u32 chars;
    u32 palette;
    u32 cellAnims;
    u8 animFrame;
    s8 animOffset;
    u8 partyIndex;
    u8 unk4F;
    u32 voice;
    // Set by the microphone's callback once the recording is done
    u32 recorded;
};

GameEvent *EventChatotRecord_Create(GameSystem *gsys, Field *field, u8 partyIndex);
GameEventReturnCode EventChatotRecord_Main(GameEvent *event, u32 *state, void *data);
void EventChatotRecord_OnRecorded(u32 result, u32 *done);
void EventChatotRecord_PauseActors(ChatotEventWork *work);
void EventChatotRecord_ResumeActors(ChatotEventWork *work);
void EventChatotRecord_CreatePokeIcon(ChatotEventWork *work);
void EventChatotRecord_DeletePokeIcon(ChatotEventWork *work);
u32 EventChatotRecord_HopPokeIcon(ChatotEventWork *work);
void EventChatotRecord_SetupWindow(ChatotEventWork *work);
void EventChatotRecord_Cleanup(ChatotEventWork *work);

#endif // POKEBW2_FIELD_EVENT_CHATOT_H
