#include "nitro/fs.h"
#include "../filesystem.h"
#include "../heap.h"
#include "gfl/heap.h"
#include "gfl/heapsys.h"
#include "gfl/std.h"
#include "nitro/host_g2d.h"
#include "nnsys/fnd.h"
#include <SDL3/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static const char *resource_root;
typedef struct OpenStream {
    struct OpenStream *next;
    SDL_IOStream *stream;
} OpenStream;
static OpenStream *openStreams;

static SDL_IOStream *TrackStream(SDL_IOStream *stream) {
    if (!stream)
        return NULL;
    OpenStream *record = SDL_malloc(sizeof(*record));
    if (!record) {
        SDL_CloseIO(stream);
        return NULL;
    }
    *record = (OpenStream){ openStreams, stream };
    openStreams = record;
    return stream;
}

void PcFs_Shutdown(void) {
    while (openStreams) {
        OpenStream *record = openStreams;
        openStreams = record->next;
        SDL_CloseIO(record->stream);
        SDL_free(record);
    }
    resource_root = NULL;
}

void PcFs_SetRoot(const char *root) {
    resource_root = root;
}

void FS_InitFile(FSFile *file) {
    SDL_zero(*file);
}
BOOL FS_OpenFile(FSFile *file, const char *path) {
    char *resolved = NULL;
    if (SDL_strncmp(path, "rom:/", 5) == 0)
        path += 5;
    if (resource_root && path[0] != '/' && !(path[0] && path[1] == ':')) {
        if (SDL_asprintf(&resolved, "%s/%s", resource_root, path) < 0)
            return FALSE;
    }
    file->hostStream = TrackStream(SDL_IOFromFile(resolved ? resolved : path, "rb"));
    SDL_free(resolved);
    file->error = file->hostStream ? FS_RESULT_SUCCESS : FS_RESULT_FAILURE;
    if (!file->hostStream)
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Opening %s: %s", path, SDL_GetError());
    return file->hostStream != NULL;
}
BOOL FS_CloseFile(FSFile *file) {
    for (OpenStream **link = &openStreams; *link; link = &(*link)->next) {
        if ((*link)->stream == file->hostStream) {
            OpenStream *record = *link;
            *link = record->next;
            SDL_free(record);
            break;
        }
    }
    bool result = !file->hostStream || SDL_CloseIO(file->hostStream);
    file->hostStream = NULL;
    return result;
}
BOOL FS_CreateFileFromMemory(FSFile *file, void *data, u32 size) {
    file->hostStream = TrackStream(SDL_IOFromConstMem(data, size));
    return file->hostStream != NULL;
}
s32 FS_ReadFile(FSFile *file, void *dest, s32 size) {
    if (size < 0 || !file->hostStream)
        return -1;
    size_t read = SDL_ReadIO(file->hostStream, dest, (size_t)size);
    file->error = read == (size_t)size ? FS_RESULT_SUCCESS : FS_RESULT_FAILURE;
    if (read != (size_t)size)
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Short resource read: %zu/%d", read, size);
    return (s32)read;
}
BOOL FS_SeekFile(FSFile *file, s32 offset, FSSeekFileMode whence) {
    SDL_IOWhence mode = whence == FS_SEEK_SET   ? SDL_IO_SEEK_SET
                        : whence == FS_SEEK_CUR ? SDL_IO_SEEK_CUR
                                                : SDL_IO_SEEK_END;
    return SDL_SeekIO(file->hostStream, offset, mode) >= 0;
}
u32 FS_GetFilePosition(FSFile *file) {
    return (u32)SDL_TellIO(file->hostStream);
}
u32 FS_GetFileLength(FSFile *file) {
    return (u32)SDL_GetIOSize(file->hostStream);
}
u32 FS_GetLength(FSFile *file) {
    return FS_GetFileLength(file);
}

typedef struct HeapAllocation {
    struct HeapAllocation *next;
    void *memory;
    HeapID heap;
    u32 size;
    bool aligned;
} HeapAllocation;
static HeapAllocation *heap_allocations;
static struct {
    BOOL active;
    HeapID parent;
    u64 capacity;
} heaps[HEAPID_TAIL_BIT];

static void FreeAllocation(HeapAllocation *allocation) {
    PcG2d_Release(allocation->memory);
    if (allocation->aligned)
        SDL_aligned_free(allocation->memory);
    else
        SDL_free(allocation->memory);
    SDL_free(allocation);
}

void PcHeap_Shutdown(void) {
    while (heap_allocations) {
        HeapAllocation *allocation = heap_allocations;
        heap_allocations = allocation->next;
        FreeAllocation(allocation);
    }
    SDL_zero(heaps);
}

void GFL_HeapCreateChild(HeapID parentHeapId, HeapID heapId, u32 size) {
    heapId &= HEAPID_TAIL_BIT - 1;
    if (heaps[heapId].active)
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Heap %u already exists", heapId);
    heaps[heapId].active = TRUE;
    heaps[heapId].parent = parentHeapId & (HEAPID_TAIL_BIT - 1);
    // Preserve the game's cache-sizing budget while allowing wider native structures.
    heaps[heapId].capacity = (u64)size * sizeof(void *) / 4;
}
void GFL_HeapCreateRoot(void *memory, u32 size, HeapID heapId) {
    (void)memory; // The original static arena has DS-sized storage; native allocations have separate ownership.
    GFL_HeapCreateChild(heapId, heapId, size);
}
u32 GFL_HeapGetFreeSize(HeapID heapId) {
    heapId &= HEAPID_TAIL_BIT - 1;
    u64 used = 0;
    for (HeapAllocation *allocation = heap_allocations; allocation; allocation = allocation->next)
        if (allocation->heap == heapId)
            used += allocation->size;
    return used >= heaps[heapId].capacity ? 0 : (u32)(heaps[heapId].capacity - used);
}
u32 GFL_HeapGetHighestAllocatableSize(HeapID heapId) {
    // Host allocations do not share a fragmented DS arena.
    return GFL_HeapGetFreeSize(heapId);
}
void GFL_HeapDumpOnFailure(HeapID heapId) {
    SDL_Log("Native heap %u: %u bytes of cache budget free", heapId, GFL_HeapGetFreeSize(heapId));
}
BOOL GFL_HeapStatusValidate(HeapID heapId) {
    heapId &= HEAPID_TAIL_BIT - 1;
    if (!heaps[heapId].active)
        return FALSE;
    // Native blocks use the host allocator; validate the ownership records instead of DS arena headers.
    for (HeapAllocation *record = heap_allocations; record; record = record->next)
        if (record->heap == heapId && !record->memory)
            GFL_DebugAssertFailEx(__FILE__, __LINE__, "Invalid allocation record in heap %u", heapId);
    return TRUE;
}
void GFL_HeapResize(void *ptr, u32 size) {
    for (HeapAllocation *allocation = heap_allocations; allocation; allocation = allocation->next) {
        if (allocation->memory == ptr) {
            if (size > allocation->size)
                GFL_DebugAssertFailEx(__FILE__, __LINE__, "Native in-place heap growth is unavailable");
            // Shrinking preserves the address, as the game's allocator promises.
            allocation->size = size;
            return;
        }
    }
    GFL_DebugAssertFailEx(__FILE__, __LINE__, "Resizing an unowned allocation");
}
void GFL_HeapDelete(HeapID heapId) {
    heapId &= HEAPID_TAIL_BIT - 1;
    for (unsigned i = 0; i < HEAPID_TAIL_BIT; i++)
        if (i != heapId && heaps[i].active && heaps[i].parent == heapId)
            GFL_HeapDelete((HeapID)i);
    HeapAllocation **link = &heap_allocations;
    while (*link) {
        HeapAllocation *allocation = *link;
        if (allocation->heap == heapId) {
            *link = allocation->next;
            FreeAllocation(allocation);
        } else
            link = &allocation->next;
    }
    heaps[heapId].active = FALSE;
}
void *GFL_HeapAllocate(HeapID heapId, u32 size, BOOL clear, const char *file, u16 line) {
    void *allocation = clear ? SDL_calloc(1, size) : SDL_malloc(size);
    HeapAllocation *record = SDL_malloc(sizeof(*record));
    if (!allocation || !record)
        GFL_DebugAssertFailEx(file, line, "Unable to allocate %u bytes", size);
    *record = (HeapAllocation){ heap_allocations, allocation, heapId & (HEAPID_TAIL_BIT - 1), size, false };
    heap_allocations = record;
    return allocation;
}
void GFL_HeapFree(void *ptr) {
    for (HeapAllocation **link = &heap_allocations; *link; link = &(*link)->next) {
        if ((*link)->memory == ptr) {
            HeapAllocation *record = *link;
            *link = record->next;
            FreeAllocation(record);
            return;
        }
    }
    PcG2d_Release(ptr);
    SDL_free(ptr);
}
BOOL GFL_HeapFreeCore(void *ptr) {
    for (HeapAllocation *record = heap_allocations; record; record = record->next) {
        if (record->memory == ptr) {
            GFL_HeapFree(ptr);
            return TRUE;
        }
    }
    return FALSE;
}
static void *AllocateAligned(NNSFndAllocator *allocator, u32 size) {
    void *memory = SDL_aligned_alloc(allocator->heapParam1, size);
    HeapAllocation *record = SDL_malloc(sizeof(*record));
    if (!memory || !record) {
        SDL_aligned_free(memory);
        SDL_free(record);
        return NULL;
    }
    *record = (HeapAllocation){ heap_allocations, memory, allocator->heapParam2 & (HEAPID_TAIL_BIT - 1), size, true };
    heap_allocations = record;
    return memory;
}
static void FreeAligned(NNSFndAllocator *allocator, void *memory) {
    (void)allocator;
    GFL_HeapFree(memory);
}
void GFL_HeapCreateAllocator(NNSFndAllocator *allocator, HeapID heapId, int alignment) {
    static const NNSFndAllocatorFunc callbacks = { AllocateAligned, FreeAligned };
    unsigned magnitude = alignment < 0 ? (unsigned)-(s64)alignment : (unsigned)alignment;
    if (magnitude < sizeof(void *))
        magnitude = sizeof(void *);
    if (magnitude & (magnitude - 1))
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Invalid allocator alignment");
    *allocator = (NNSFndAllocator){ .pFunc = &callbacks, .heapParam1 = magnitude, .heapParam2 = heapId };
}
// A host command buffer has ordinary coherent memory; it does not need the ARM's dedicated DTCM region.
void *GFL_HeapDTCMAllocate(u32 size) {
    return GFL_HeapAllocate(HEAPID_SYSTEM, size, FALSE, __FILE__, __LINE__);
}
void _freeBlkFromDTCM(void *ptr) {
    GFL_HeapFree(ptr);
}
void GFL_DebugAssertFail(const char *file, u32 line, const char *expression) {
    GFL_DebugAssertFailEx(file, line, "%s", expression);
}
void GFL_DebugAssertFailEx(const char *file, u32 line, const char *format, ...) {
    va_list args;
    fprintf(stderr, "BW2 assertion %s:%u: ", file, line);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);
    abort();
}
