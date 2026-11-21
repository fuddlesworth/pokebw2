#ifndef POKEBW2_COMMUNICATION_XTRANSCEIVER_CTVT_MIC_H
#define POKEBW2_COMMUNICATION_XTRANSCEIVER_CTVT_MIC_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The Xtransceiver's microphone: recording the voice, packing it with IMA ADPCM, and playing the other side's voice

CtvtMic *CtvtMic_Create(HeapID heapId);
void CtvtMic_Delete(CtvtMic *work);
void CtvtMic_Update(CtvtMic *work);
void CtvtMic_Draw(CtvtMic *work);
BOOL CtvtMic_StartRecording(CtvtMic *work);
BOOL CtvtMic_StopRecording(CtvtMic *work);
BOOL CtvtMic_IsRecording(CtvtMic *work);
u32 CtvtMic_GetRecordedSize(CtvtMic *work);
void *CtvtMic_GetBuffer(CtvtMic *work);
u32 CtvtMic_Encode(CtvtMic *work, const s16 *src, u8 *dst, u32 size);
u32 CtvtMic_Decode(CtvtMic *work, const s8 *src, s16 *dst, u32 size);
void CtvtMic_Debug(CtvtMic *work);
BOOL CtvtMic_Play(CtvtMic *work, const void *data, u32 size, int volume, int speed);
void CtvtMic_StopPlaying(CtvtMic *work);
BOOL CtvtMic_IsPlaying(CtvtMic *work);
u32 CtvtMic_GetPlaySize(CtvtMic *work);
u16 CtvtMic_GetPlayFrames(CtvtMic *work);
u16 CtvtMic_GetPlayLength(CtvtMic *work);
BOOL CtvtMic_IsReady(CtvtMic *work);

#endif // POKEBW2_APP_COMM_TVT_CTVT_MIC_H
