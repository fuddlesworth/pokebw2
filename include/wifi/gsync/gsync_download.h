#ifndef POKEBW2_APP_GSYNC_GSYNC_DOWNLOAD_H
#define POKEBW2_APP_GSYNC_GSYNC_DOWNLOAD_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Game Sync's download of a file from the Dream World's server (gsync_download.c), through NitroDWC's download
// library. Only one exists at a time. The asynchronous calls return whether they started

GSyncDownload *GSyncDownload_Create(HeapID heapId, u32 size);
void GSyncDownload_Free(GSyncDownload *dl);
void *GSyncDownload_GetBuffer(GSyncDownload *dl);
BOOL GSyncDownload_Init(GSyncDownload *dl);
// Whether the last call ended without an error, or with one or by timing out
BOOL GSyncDownload_IsSucceeded(GSyncDownload *dl);
BOOL GSyncDownload_IsFailed(GSyncDownload *dl);
// Sets the attributes the files are listed by: a string and a number
BOOL GSyncDownload_SetAttr(GSyncDownload *dl, const char *attr1, int attr2);
void GSyncDownload_ResetTimer(GSyncDownload *dl);
BOOL GSyncDownload_GetFileList(GSyncDownload *dl);
BOOL GSyncDownload_GetFile(GSyncDownload *dl);
BOOL GSyncDownload_Cleanup(GSyncDownload *dl);
void GSyncDownload_Main(GSyncDownload *dl);
u32 GSyncDownload_GetFileSize(GSyncDownload *dl);

#endif // POKEBW2_APP_GSYNC_GSYNC_DOWNLOAD_H
