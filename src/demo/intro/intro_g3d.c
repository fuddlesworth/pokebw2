#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "demo/intro/intro.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/sound.h"
#include "nitro/hw.h"

// The intro's 3D model, which is only loaded in the parts of the intro that show it

#define SCENE_NONE 0xffff

struct IntroG3d {
    IntroGraphic *graphic;
    HeapID heapId;
    G3DManager *manager;
    G3DCamera *camera;
    u16 scene;
    BOOL visible;
    BOOL active;
    u32 mode;
    u32 animFinished;
    u32 state;
    u32 frame;
    u32 padding2C;
};

static const VecFx32 sPosition = { 0, 0, FX32_ONE };
static const VecFx32 sUpVector = { 0, FX32_ONE, 0 };
static const VecFx32 sTarget = { 0, 0, -FX32_ONE };

// Resource-table slots and actor animation slots are separate domains.
enum {
    INTRO_G3D_RESOURCE_MODEL = 0,
    INTRO_G3D_RESOURCE_GENDER_JOINT,
    INTRO_G3D_RESOURCE_GENDER_MATERIAL,
    INTRO_G3D_RESOURCE_GENDER_TEXTURE_PATTERN,
    INTRO_G3D_RESOURCE_OPEN_JOINT,
    INTRO_G3D_RESOURCE_OPEN_MATERIAL,
};

enum {
    INTRO_G3D_ANIM_GENDER_JOINT = 0,
    INTRO_G3D_ANIM_GENDER_MATERIAL,
    INTRO_G3D_ANIM_GENDER_TEXTURE_PATTERN,
    INTRO_G3D_ANIM_OPEN_JOINT,
    INTRO_G3D_ANIM_OPEN_MATERIAL,
};

// The model and its animations, which differ by version
// clang-format off
static const G3DSceneResourceSetup sResources[] = {
#ifdef BLACK2
    { ARCID_INTRO, 22, 0 }, { ARCID_INTRO, 16, 0 }, { ARCID_INTRO, 17, 0 }, { ARCID_INTRO, 18, 0 }, { ARCID_INTRO, 12, 0 }, { ARCID_INTRO, 13, 0 },
#else
    { ARCID_INTRO, 23, 0 }, { ARCID_INTRO, 19, 0 }, { ARCID_INTRO, 20, 0 }, { ARCID_INTRO, 21, 0 }, { ARCID_INTRO, 14, 0 }, { ARCID_INTRO, 15, 0 },
#endif
};

// clang-format on

static const G3DSceneAnimationSetup sAnimations[] = {
    { INTRO_G3D_RESOURCE_GENDER_JOINT, 0 },           { INTRO_G3D_RESOURCE_GENDER_MATERIAL, 0 },
    { INTRO_G3D_RESOURCE_GENDER_TEXTURE_PATTERN, 0 }, { INTRO_G3D_RESOURCE_OPEN_JOINT, 0 },
    { INTRO_G3D_RESOURCE_OPEN_MATERIAL, 0 },
};

static const G3DSceneActorSetup sActors[] = {
    { INTRO_G3D_RESOURCE_MODEL, 0, INTRO_G3D_RESOURCE_MODEL, 0, sAnimations, NELEMS(sAnimations) },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

IntroG3d *IntroG3d_Create(IntroGraphic *graphic, u32 mode, HeapID heapId) {
    IntroG3d *g3d = GFL_HeapAllocate(heapId, sizeof(IntroG3d), TRUE, "intro_g3d.c", 176);
    VecFx32 position;
    VecFx32 upVector;
    VecFx32 target;
    G3DActor *actor;

    g3d->heapId = heapId;
    g3d->graphic = graphic;
    g3d->manager = GFL_G3DMgrCreate(10, 16, heapId);
    position = sPosition;
    upVector = sUpVector;
    target = sTarget;
    g3d->camera =
        GFL_G3DCameraCreate(G3DCAM_PROJECTION_ORTHO, FX32_CONST(6), -FX32_CONST(6), -FX32_CONST(8), FX32_CONST(8),
                            FX32_ONE, FX32_CONST(1024), FX32_ONE, &position, &upVector, &target, heapId);
    if (mode != INTRO_MODE_PLAYER_NAMED && mode != INTRO_MODE_RIVAL_NAMED) {
        g3d->scene = GFL_G3DMgrNewScene(g3d->manager, &sSceneSetup);
        actor = GFL_G3DMgrGetActor(g3d->manager, g3d->scene);
        GFL_G3DActorBindAnm(actor, INTRO_G3D_ANIM_OPEN_JOINT);
        GFL_G3DActorBindAnm(actor, INTRO_G3D_ANIM_OPEN_MATERIAL);
        GFL_G3DActorUnbindAnm(actor, INTRO_G3D_ANIM_GENDER_JOINT);
        GFL_G3DActorUnbindAnm(actor, INTRO_G3D_ANIM_GENDER_MATERIAL);
        GFL_G3DActorUnbindAnm(actor, INTRO_G3D_ANIM_GENDER_TEXTURE_PATTERN);
    } else {
        g3d->scene = SCENE_NONE;
    }
    g3d->active = TRUE;
    return g3d;
}

void IntroG3d_Free(IntroG3d *g3d) {
    GFL_G3DCameraFree(g3d->camera);
    if (g3d->scene != SCENE_NONE) {
        GFL_G3DMgrDeleteScene(g3d->manager, g3d->scene);
    }
    GFL_G3DMgrFree(g3d->manager);
    GFL_HeapFree(g3d);
}

void IntroG3d_Draw(IntroG3d *g3d) {
    SRTMatrix transform;

    GFL_G3DCameraFlush(g3d->camera);
    if (g3d->visible && g3d->active) {
        transform.translation.x = 0;
        transform.translation.y = 0;
        transform.translation.z = 0;
        transform.scale.x = FX32_ONE;
        transform.scale.y = FX32_ONE;
        transform.scale.z = FX32_ONE;
        MTX_Identity33_(&transform.rotation);
        GFL_G3DSysDrawObj(GFL_G3DMgrGetActor(g3d->manager, g3d->scene), &transform);
    }
}

enum {
    STEP_G3D_OPEN_START,
    STEP_G3D_OPEN_ANIM,
    STEP_G3D_OPEN_DONE,
};

BOOL IntroG3d_Open(IntroG3d *g3d) {
    G3DActor *actor;
    BOOL playingOpenJoint;
    BOOL playingOpenMaterial;

    switch (g3d->state) {
    case STEP_G3D_OPEN_START:
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0, (GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG3),
                           0, 0);
        GFL_SndSEPlay(SEQ_SE_OPEN2);
        g3d->state++;
    case STEP_G3D_OPEN_ANIM:
        actor = GFL_G3DMgrGetActor(g3d->manager, g3d->scene);
        playingOpenJoint = GFL_G3DActorStepAnmFrame(actor, INTRO_G3D_ANIM_OPEN_JOINT, FX32_ONE);
        playingOpenMaterial = GFL_G3DActorStepAnmFrame(actor, INTRO_G3D_ANIM_OPEN_MATERIAL, FX32_ONE);
        if (!playingOpenJoint && !playingOpenMaterial) {
            GFL_G3DActorUnbindAnm(actor, INTRO_G3D_ANIM_OPEN_JOINT);
            GFL_G3DActorUnbindAnm(actor, INTRO_G3D_ANIM_OPEN_MATERIAL);
            GFL_G3DActorBindAnm(actor, INTRO_G3D_ANIM_GENDER_JOINT);
            GFL_G3DActorBindAnm(actor, INTRO_G3D_ANIM_GENDER_MATERIAL);
            GFL_G3DActorBindAnm(actor, INTRO_G3D_ANIM_GENDER_TEXTURE_PATTERN);
            IntroG3d_SetFrame(g3d, 60);
            g3d->state++;
        }
        break;
    case STEP_G3D_OPEN_DONE:
        return TRUE;
    }
    return FALSE;
}

void IntroG3d_SetVisible(IntroG3d *g3d, BOOL visible) {
    g3d->visible = visible;
}

void IntroG3d_SetMode(IntroG3d *g3d, u32 mode) {
    g3d->mode = mode;
    g3d->animFinished = 0;
    switch (mode) {
    case INTRO_G3D_MODE_INITIAL:
        g3d->frame = 60;
        g3d->mode = INTRO_G3D_MODE_PREVIEW_MALE;
        break;
    case INTRO_G3D_MODE_PREVIEW_MALE:
        g3d->frame = 70;
        break;
    case INTRO_G3D_MODE_PREVIEW_FEMALE:
        g3d->frame = 50;
        break;
    case INTRO_G3D_MODE_CONFIRM_MALE:
        g3d->frame = 50;
        break;
    case INTRO_G3D_MODE_CONFIRM_FEMALE:
        g3d->frame = 70;
        break;
    }
}

BOOL IntroG3d_Animate(IntroG3d *g3d) {
    switch (g3d->mode) {
    case INTRO_G3D_MODE_INITIAL:
        break;
    case INTRO_G3D_MODE_PREVIEW_MALE:
        g3d->frame--;
        IntroG3d_SetFrame(g3d, g3d->frame);
        if (g3d->frame == 50) {
            return TRUE;
        }
        break;
    case INTRO_G3D_MODE_PREVIEW_FEMALE:
        g3d->frame++;
        IntroG3d_SetFrame(g3d, g3d->frame);
        if (g3d->frame == 70) {
            return TRUE;
        }
        break;
    case INTRO_G3D_MODE_CONFIRM_MALE:
        g3d->frame--;
        IntroG3d_SetFrame(g3d, g3d->frame);
        if (g3d->frame == 0) {
            return TRUE;
        }
        break;
    case INTRO_G3D_MODE_CONFIRM_FEMALE:
        g3d->frame++;
        IntroG3d_SetFrame(g3d, g3d->frame);
        if (g3d->frame == 120) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL IntroG3d_AnimateBack(IntroG3d *g3d) {
    if (g3d->mode == INTRO_G3D_MODE_CONFIRM_MALE) {
        IntroG3d_SetFrame(g3d, ++g3d->frame);
    } else {
        IntroG3d_SetFrame(g3d, --g3d->frame);
    }
    if (g3d->frame == 60) {
        return TRUE;
    }
    return FALSE;
}

void IntroG3d_SetFrame(IntroG3d *g3d, u32 frame) {
    G3DActor *actor = GFL_G3DMgrGetActor(g3d->manager, g3d->scene);
    fx32 value = FX32_CONST(frame);

    GFL_G3DActorSetAnmFrame(actor, INTRO_G3D_ANIM_GENDER_JOINT, &value);
    GFL_G3DActorSetAnmFrame(actor, INTRO_G3D_ANIM_GENDER_MATERIAL, &value);
    GFL_G3DActorSetAnmFrame(actor, INTRO_G3D_ANIM_GENDER_TEXTURE_PATTERN, &value);
}
