#include "../filesystem.h"
#include "../heap.h"
#include "check.h"
#include "gfl/heap.h"
#include "nitro/fs.h"
#include "nnsys/fnd.h"
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#ifndef _WIN32
#include <errno.h>
#include <fcntl.h>
#endif

int main(int argc, char **argv) {
    CHECK(argc == 2);
    for (unsigned cycle = 0; cycle < 3; cycle++) {
        GFL_HeapCreateRoot(NULL, 4096, HEAPID_SYSTEM);
        GFL_HeapCreateChild(HEAPID_SYSTEM, HEAPID_USER, 2048);
        GFL_HeapCreateRoot(NULL, 2048, HEAPID_FIELD_CLACT);
        FSFile *file = GFL_HeapAllocate(HEAPID_USER, sizeof(*file), TRUE, __FILE__, __LINE__);
        CHECK(FS_OpenFile(file, argv[1]));
#ifndef _WIN32
        int fd = (int)SDL_GetNumberProperty(SDL_GetIOProperties(file->hostStream),
                                            SDL_PROP_IOSTREAM_FILE_DESCRIPTOR_NUMBER, -1);
        CHECK(fd >= 0);
#endif
        char *memory = GFL_HeapAllocate(HEAPID_FIELD_CLACT, 128, TRUE, __FILE__, __LINE__);
        FSFile memoryFile;
        FS_InitFile(&memoryFile);
        CHECK(FS_CreateFileFromMemory(&memoryFile, memory, 128));
        FSFile closedFile;
        FS_InitFile(&closedFile);
        CHECK(FS_OpenFile(&closedFile, argv[1]));
        CHECK(FS_CloseFile(&closedFile));
        CHECK(FS_CloseFile(&closedFile));
        NNSFndAllocator allocator;
        GFL_HeapCreateAllocator(&allocator, HEAPID_USER, 32);
        void *aligned = NNS_FndAllocFromAllocator(&allocator, 96);
        CHECK(aligned && (uintptr_t)aligned % 32 == 0);
        NNS_FndFreeToAllocator(&allocator, aligned);
        CHECK(NNS_FndAllocFromAllocator(&allocator, 160));
        // Deliberately leave streams and both root/child allocations live, as an interrupted game does.
        PcFs_Shutdown();
#ifndef _WIN32
        errno = 0;
        CHECK(fcntl(fd, F_GETFD) == -1 && errno == EBADF);
#endif
        PcHeap_Shutdown();
        PcFs_Shutdown();
        PcHeap_Shutdown();
    }
    puts("Host cleanup closes live files and releases independent, child and aligned allocations");
    return 0;
}
