#ifndef POKEBW2_APP_MUSICAL_STA_ACT_EFFECT_H
#define POKEBW2_APP_MUSICAL_STA_ACT_EFFECT_H

// Overlay 209's sta_act_effect.c: the stage's particle effects, each a particle system with up to four emitters

#include "types.h"
#include "gfl/heap.h"
#include "gfl/particle.h"
#include "nitro/fx.h"
#include "struct_decls.h"

#define STA_ACT_EFFECT_MAX 8
#define STA_ACT_EFFECT_EMITTER_MAX 4

typedef struct {
    SPLEmitter *emitter;
    VecFx32 pos;
} StaActEffectEmitter;

struct StaActEffect {
    BOOL active;
    u8 ptcWork[0x4800];
    ParticleSystem *ptc;
    StaActEffectEmitter emitters[STA_ACT_EFFECT_EMITTER_MAX];
};

struct StaActEffectSys {
    HeapID heapId;
    StaActEffect effects[STA_ACT_EFFECT_MAX];
};

StaActEffectSys *StaActEffect_InitSystem(HeapID heapId);
void StaActEffect_TermSystem(StaActEffectSys *sys);
void StaActEffect_UpdateSystem(StaActEffectSys *sys);
void StaActEffect_DrawSystem(StaActEffectSys *sys);
// Adds an effect with the particle resource of ARCID_MUSICAL's file fileId
StaActEffect *StaActEffect_AddEffect(StaActEffectSys *sys, u32 fileId);
void StaActEffect_DelEffect(StaActEffectSys *sys, StaActEffect *effect);
void StaActEffect_CreateEmitter(StaActEffect *effect, u16 emitterNo, VecFx32 *pos);
void StaActEffect_DeleteEmitter(StaActEffect *effect, u16 emitterNo);
void StaActEffect_SetPosition(StaActEffect *effect, u16 emitterNo, VecFx32 *pos);

#endif // POKEBW2_APP_MUSICAL_STA_ACT_EFFECT_H
