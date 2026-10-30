#ifndef POKEBW2_FIELD_EVENT_SEASON_BANNER_H
#define POKEBW2_FIELD_EVENT_SEASON_BANNER_H

#include "types.h"
#include "struct_decls.h"

#ifdef POKEBW2_PC
BOOL EventSeasonBanner_IsNativeActive(void);
#endif

typedef void (*EventSeasonBannerCallback)(void *arg);

GameEvent *EventSeasonBanner_CreateFieldTransition(GameSystem *gsys, Field *field, u8 startSeason, u8 endSeason);
GameEvent *EventSeasonBanner_CreateFieldTransitionEx(GameSystem *gsys, Field *field, u8 startSeason, u8 endSeason,
                                                     EventSeasonBannerCallback callback, void *callbackArg);
GameEvent *EventSeasonBanner_CreateStandalone(GameSystem *gsys, u8 startSeason, u8 endSeason);

#endif // POKEBW2_FIELD_EVENT_SEASON_BANNER_H
