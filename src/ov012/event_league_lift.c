// The Pokémon League's lift down to the Champion's room: the camera turns, the lift goes down with the player, the
// zone changes, and the lift lands with a shake. The name is descriptive; EventLeagueLiftWarp_Callback and
// EventLeagueLiftWarp_Create are swan's names (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/directions.h"
#include "constants/sound.h"
#include "constants/zones.h"
#include "field/event_league_lift.h"
#include "field/event_mapchange.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/field_event.h"
#include "field/field_exp_obj.h"
#include "field/gimmick_league_lift.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/game_event.h"

typedef struct {
    GameSystem *gsys;
    Field *field;
    FieldCamera *camera;
    FieldTaskManager *taskManager;
    G3DCurve *curve;
    FieldCameraAnimation animation;
    FieldCameraAnimationController *cameraController;
} LeagueLiftWarpData;

typedef struct {
    u16 heapId;
    GameSystem *gsys;
    Field *field;
    FieldCamera *camera;
    G3DCurve *curve;
} LeagueLiftArrivalData;

// Where the camera turns to before the lift goes down
static const FieldCameraCoords sLeagueLiftCameraCoords = {
    0x1d7d, 0, 0x16c000, { 0x1f8000, -0x40000, 0x264000 }, { 0, 0x9d000, 0x90000 },
};

static GameEvent *EventLeagueLiftArrival_Create(GameSystem *gsys, Field *field);

// EventLeagueLiftWarp_Callback's steps: the camera's turn, the lift's curve and its descent, the fade and the map
// change, and the arrival
enum {
    LEAGUE_LIFT_WARP_STEP_PREPARE_CAMERA,
    LEAGUE_LIFT_WARP_STEP_WAIT_CAMERA,
    LEAGUE_LIFT_WARP_STEP_LOAD_CURVE,
    LEAGUE_LIFT_WARP_STEP_LOWER,
    LEAGUE_LIFT_WARP_STEP_FADE_OUT,
    LEAGUE_LIFT_WARP_STEP_MAP_CHANGE,
    LEAGUE_LIFT_WARP_STEP_ARRIVAL,
    LEAGUE_LIFT_WARP_STEP_END,
};

static GameEventReturnCode EventLeagueLiftWarp_Callback(GameEvent *event, u32 *state, void *work) {
    LeagueLiftWarpData *data = work;
    GameSystem *gsys = data->gsys;
    Field *field = data->field;
    FieldTaskManager *taskManager = data->taskManager;
    VecFx32 translation;
    VecFx32 pos;
    FieldPlayer *player;
    VecFx32 spawnPos;

    switch (*state) {
    case LEAGUE_LIFT_WARP_STEP_PREPARE_CAMERA:
        FieldCameraAnimationController_PrepareCamera(data->cameraController);
        FieldCamera_GetAnimationCoords(data->camera, &data->animation.src);
        FieldCameraAnimationController_StartAnimation(data->cameraController);
        (*state)++;
        break;
    case LEAGUE_LIFT_WARP_STEP_WAIT_CAMERA:
        if (FieldTaskManager_IsIdle(taskManager)) {
            FieldCameraAnimationController_FreeAnimation(data->cameraController);
            (*state)++;
        }
        break;
    case LEAGUE_LIFT_WARP_STEP_LOAD_CURVE:
        data->curve = GFL_G3DCurveLoadFileAll(Field_GetHeapID(field), 0x9b, 0x18);
        GFL_SEPlayKeepVol(SEQ_SE_FLD_148, GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_148));
        (*state)++;
        break;
    case LEAGUE_LIFT_WARP_STEP_LOWER:
        GFL_G3DCurveGetNowTranslation(data->curve, &translation);
        FieldExpObj_GetActorMatrixPtr(Field_GetExpObjSystem(field), 0, 0)->translation.y = translation.y;
        player = Field_GetPlayer(field);
        FieldPlayer_GetWPos(player, &pos);
        pos.y = translation.y;
        FieldPlayer_SetWPos(player, &pos);
        if (GFL_G3DCurveFrameStepLoop(data->curve, FX32_ONE)) {
            GFL_G3DCurveFree(data->curve);
            (*state)++;
        }
        break;
    case LEAGUE_LIFT_WARP_STEP_FADE_OUT:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0));
        (*state)++;
        break;
    case LEAGUE_LIFT_WARP_STEP_MAP_CHANGE:
        spawnPos.x = 0;
        spawnPos.y = 0;
        spawnPos.z = 0;
        GameEvent_ChainNext(event,
                            EventMapChange_CreateGridDefault(gsys, field, ZONE_POKEMON_LEAGUE_3, &spawnPos, DIR_DOWN));
        (*state)++;
        break;
    case LEAGUE_LIFT_WARP_STEP_ARRIVAL:
        GameEvent_Replace(event, EventLeagueLiftArrival_Create(gsys, field));
        (*state)++;
        break;
    case LEAGUE_LIFT_WARP_STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventLeagueLiftWarp_Create(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventLeagueLiftWarp_Callback, sizeof(LeagueLiftWarpData));
    LeagueLiftWarpData *data = GameEvent_GetData(event);

    data->gsys = gsys;
    data->field = field;
    data->curve = NULL;
    data->camera = Field_GetCameraSystem(field);
    data->taskManager = Field_GetTaskManager(field);
    data->animation.frames = 20;
    data->animation.dst = sLeagueLiftCameraCoords;
    data->animation.releaseBind = TRUE;
    data->animation.disableBoundary = TRUE;
    data->cameraController = Field_CreateCameraAnimationController(field);
    FieldCameraAnimationController_SetAnimation(data->cameraController, &data->animation);
    return event;
}

static BOOL LeagueLiftArrival_StepCurve(LeagueLiftArrivalData *data) {
    return GFL_G3DCurveFrameStepLoop(data->curve, FX32_ONE);
}

// Moves the lift to the curve
static void LeagueLiftArrival_MoveLift(LeagueLiftArrivalData *data) {
    VecFx32 translation;

    GFL_G3DCurveGetNowTranslation(data->curve, &translation);
    FieldExpObj_GetActorMatrixPtr(Field_GetExpObjSystem(data->field), 0, 0)->translation.y = translation.y;
}

// Puts the player on the lift, in the middle of its square
static void LeagueLiftArrival_PlacePlayer(LeagueLiftArrivalData *data) {
    SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(Field_GetExpObjSystem(data->field), 0, 0);
    FieldPlayer *player = Field_GetPlayer(data->field);
    VecFx32 pos;

    VEC_Set(&pos, matrix->translation.x, matrix->translation.y, matrix->translation.z + FX32_CONST(32));
    pos.x = FX_Whole(pos.x) / 16 * FX32_CONST(16) + FX32_CONST(8);
    pos.z = FX_Whole(pos.z) / 16 * FX32_CONST(16) + FX32_CONST(8);
    FieldPlayer_SetWPos(player, &pos);
}

// EventLeagueLiftArrival_Callback's steps: the curve and the camera's delay, the lift's animation and the fade in,
// the ride, the landing, and the player's height
enum {
    LEAGUE_LIFT_ARRIVAL_STEP_LOAD_CURVE,
    LEAGUE_LIFT_ARRIVAL_STEP_WAIT_CAMERA,
    LEAGUE_LIFT_ARRIVAL_STEP_PLAY_ANIM,
    LEAGUE_LIFT_ARRIVAL_STEP_RIDE_CURVE,
    LEAGUE_LIFT_ARRIVAL_STEP_WAIT_ANIM,
    LEAGUE_LIFT_ARRIVAL_STEP_PLACE_ON_GROUND,
    LEAGUE_LIFT_ARRIVAL_STEP_END,
};

static GameEventReturnCode EventLeagueLiftArrival_Callback(GameEvent *event, u32 *state, void *work) {
    LeagueLiftArrivalData *data = work;
    BOOL done;
    FieldCameraShakeSetup shake;
    FieldPlayer *player;
    FieldActor *actor;
    VecFx32 pos;
    fx32 height;

    Field_GetExpObjSystem(data->field);
    switch (*state) {
    case LEAGUE_LIFT_ARRIVAL_STEP_LOAD_CURVE:
        data->curve = GFL_G3DCurveLoadFileAll(data->heapId, 0x9b, 0x19);
        LeagueLiftArrival_MoveLift(data);
        LeagueLiftArrival_PlacePlayer(data);
        FieldCamera_FinishDelay(data->camera);
        (*state)++;
        break;
    case LEAGUE_LIFT_ARRIVAL_STEP_WAIT_CAMERA:
        if (!FieldCamera_IsDelayActive(data->camera)) {
            (*state)++;
        }
        break;
    case LEAGUE_LIFT_ARRIVAL_STEP_PLAY_ANIM:
        GimmickLeagueLift_PlayLiftAnim(data->field);
        GimmickLeagueLift_PlayArrivalModel(data->field);
        GameEvent_ChainNext(
            event, CallFieldMapEntranceInTransition(data->gsys, data->field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        (*state)++;
        break;
    case LEAGUE_LIFT_ARRIVAL_STEP_RIDE_CURVE:
        done = LeagueLiftArrival_StepCurve(data);
        if (!done) {
            LeagueLiftArrival_MoveLift(data);
            LeagueLiftArrival_PlacePlayer(data);
        }
        if (done) {
            GFL_G3DCurveFree(data->curve);
            GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_148));
            GFL_SndSEPlay(SEQ_SE_FLD_94);
            shake.amplitudeX = 2;
            shake.amplitudeY = 0;
            shake.period = 3;
            shake.steps = 2;
            shake.totalSteps = 2;
            shake.frame = 0;
            shake.decayX = 0;
            shake.decayY = 1;
            shake.decayStart = 0;
            shake.decayTimer = 0;
            shake.decayInterval = 2;
            GameEvent_ChainNext(event, EventEvCameraShake_Create(data->gsys, &shake));
            (*state)++;
        }
        break;
    case LEAGUE_LIFT_ARRIVAL_STEP_WAIT_ANIM:
        if (GimmickLeagueLift_IsLiftAnimDone(data->field) && GimmickLeagueLift_IsArrivalModelDone(data->field)) {
            GimmickLeagueLift_StopLiftAnim(data->field);
            GimmickLeagueLift_HideArrivalModel(data->field);
            (*state)++;
        }
        break;
    case LEAGUE_LIFT_ARRIVAL_STEP_PLACE_ON_GROUND:
        player = Field_GetPlayer(data->field);
        actor = FieldPlayer_GetActor(player);
        FieldPlayer_GetWPos(player, &pos);
        GetHeightFromMap(actor, &pos, &height);
        pos.y = height;
        FieldPlayer_SetWPos(player, &pos);
        (*state)++;
        break;
    case LEAGUE_LIFT_ARRIVAL_STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *EventLeagueLiftArrival_Create(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventLeagueLiftArrival_Callback, sizeof(LeagueLiftArrivalData));
    LeagueLiftArrivalData *data = GameEvent_GetData(event);

    data->heapId = Field_GetHeapID(field);
    data->gsys = gsys;
    data->field = field;
    data->camera = Field_GetCameraSystem(field);
    data->curve = NULL;
    return event;
}
