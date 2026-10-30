#include "nitro/mic.h"
#include "../microphone.h"
#include "nitro/pm.h"
#include "twl/sndex.h"
#include <SDL3/SDL.h>

static SDL_AudioStream *stream;
static MICAutoParam sampling;
static u32 position;
static PMAmpSwitch amplifier;

void PcMic_Shutdown(void) {
    if (stream)
        SDL_DestroyAudioStream(stream);
    stream = NULL;
}
void MIC_Init(void) {
    PcMic_Shutdown();
    SDL_zero(sampling);
    position = 0;
}
u32 PM_SetAmp(PMAmpSwitch amp) {
    amplifier = amp;
    if (amp == PM_AMP_OFF)
        PcMic_Shutdown();
    return PM_RESULT_SUCCESS;
}
u32 PM_SetAmpGain(PMAmpGain gain) {
    // Host capture devices manage their own input gain; the DS amplifier is not present.
    return PM_RESULT_ERROR;
}
SNDEXResult SNDEXi_SetIirFilter(SNDEXIirFilterTarget target, const SNDEXIirFilterParam *param) {
    return SNDEX_RESULT_ILLEGAL_STATE;
}
MICResult MIC_StartLimitedSampling(const MICAutoParam *param) {
    if (stream)
        return MIC_RESULT_BUSY;
    if (!param || !param->buffer || !param->size || param->rate < MIC_SAMPLING_RATE_LIMIT ||
        param->type != MIC_SAMPLING_TYPE_SIGNED_8BIT)
        return MIC_RESULT_ILLEGAL_PARAMETER;
    if (amplifier != PM_AMP_ON)
        return MIC_RESULT_ILLEGAL_STATUS;
    SDL_AudioSpec spec = { .format = SDL_AUDIO_S8, .channels = 1, .freq = 33513982 / param->rate };
    stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_RECORDING, &spec, NULL, NULL);
    if (!stream || !SDL_ResumeAudioStreamDevice(stream)) {
        PcMic_Shutdown();
        return MIC_RESULT_FATAL_ERROR;
    }
    sampling = *param;
    position = 0;
    return MIC_RESULT_SUCCESS;
}
void PcMic_Update(void) {
    if (!stream)
        return;
    int received = SDL_GetAudioStreamData(stream, (u8 *)sampling.buffer + position, sampling.size - position);
    if (received < 0) {
        PcMic_Shutdown();
        if (sampling.full_callback)
            sampling.full_callback(MIC_RESULT_FATAL_ERROR, sampling.full_arg);
        return;
    }
    position += received;
    if (position == sampling.size) {
        if (sampling.loop_enable)
            position = 0;
        else
            PcMic_Shutdown();
        if (sampling.full_callback)
            sampling.full_callback(MIC_RESULT_SUCCESS, sampling.full_arg);
    }
}
