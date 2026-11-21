#ifndef POKEBW2_COMMUNICATION_XTRANSCEIVER_IMA_ADPCM_H
#define POKEBW2_COMMUNICATION_XTRANSCEIVER_IMA_ADPCM_H

#include "types.h"

// IMA ADPCM for the video chat's voice. Each direction keeps its own state, which the reset functions clear

void Adpcm_ResetEncoder(void);
void Adpcm_ResetDecoder(void);
// Encodes size bytes of 16-bit samples into dst, and returns the size of the encoded data
u32 Adpcm_Encode(const s16 *src, u32 size, u8 *dst);
// Decodes size bytes of encoded data into dst, and returns the size of the samples
u32 Adpcm_Decode(const s8 *src, u32 size, s16 *dst);

#endif // POKEBW2_APP_COMM_TVT_IMA_ADPCM_H
