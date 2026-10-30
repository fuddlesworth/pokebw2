#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "gfl/backup_card.h"
#include "gfl/backup_system.h"
#include "gfl/ui.h"
#include "nitro/host_card.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <sys/resource.h>
#include <unistd.h>
#endif

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "Backup check failed at line %d: %s (%s)\n", __LINE__, #condition, SDL_GetError());        \
            return EXIT_FAILURE;                                                                                       \
        }                                                                                                              \
    } while (0)

static unsigned read_errors;
static void ReadError(void) {
    read_errors++;
}
static u32 BlockSize(void) {
    return 48;
}
static void InitBlock(void *data) {
    SDL_memset(data, 0, BlockSize());
}
static const SaveBlockDef blocks[] = { { 0, BlockSize, InitBlock }, { 1, BlockSize, InitBlock } };
static const SaveDataParams save_params = { blocks, 2, 0, 0x1000, 0x1000, 0x31053527 };

int main(void) {
    u8 source[4096], restored[4096];
    char path[256];
    BOOL written = FALSE;
    CHECK(SDL_Init(0));
    SDL_snprintf(path, sizeof(path), "backup-test-%llu.sav", (unsigned long long)SDL_GetPerformanceCounter());
#ifndef _WIN32
    // Force a real short initial write, then retry in the same session and reopen normally.
    struct rlimit original_limit, short_limit;
    CHECK(getrlimit(RLIMIT_FSIZE, &original_limit) == 0);
    short_limit = original_limit;
    short_limit.rlim_cur = 1024;
    CHECK(setrlimit(RLIMIT_FSIZE, &short_limit) == 0);
    CHECK(PcCard_OpenBackup(path));
    u8 marker = 0x5a;
    BOOL short_write = writeFlashWaitSuccess(0, &marker, 1, NULL);
    CHECK(setrlimit(RLIMIT_FSIZE, &original_limit) == 0);
    CHECK(!short_write);
    SDL_PathInfo failed_info;
    CHECK(!SDL_GetPathInfo(path, &failed_info));
    CHECK(writeFlashWaitSuccess(0, &marker, 1, NULL));
    PcCard_CloseBackup();
    CHECK(PcCard_OpenBackup(path));
    CHECK(readFlashWaitSuccess(0, restored, 1, ReadError) && restored[0] == marker);
    PcCard_CloseBackup();
    CHECK(SDL_RemovePath(path));
#endif
    // A save appearing after open must never be truncated or overwritten by initialization.
    CHECK(PcCard_OpenBackup(path));
    SDL_IOStream *concurrent = SDL_IOFromFile(path, "wb");
    CHECK(concurrent && SDL_WriteIO(concurrent, "old", 3) == 3 && SDL_CloseIO(concurrent));
    u8 replacement = 0x12;
    CHECK(!writeFlashWaitSuccess(0, &replacement, 1, NULL));
    PcCard_CloseBackup();
    concurrent = SDL_IOFromFile(path, "rb");
    CHECK(concurrent && SDL_ReadIO(concurrent, restored, sizeof(restored)) == 3 && SDL_CloseIO(concurrent));
    CHECK(SDL_memcmp(restored, "old", 3) == 0);
    CHECK(SDL_RemovePath(path));
    CHECK(PcCard_OpenBackup(path));
    CHECK(identifyGameCardBackup());
    CHECK(readFlashWaitSuccess(0, restored, sizeof(restored), ReadError));
    for (unsigned i = 0; i < sizeof(source); i++) {
        CHECK(restored[i] == 0xff);
        source[i] = (u8)(i ^ (i >> 8));
    }
    u16 lock = startWriteFlash(0x25f80, source, sizeof(source), NULL);
    CHECK(getLockIDStatus());
    CHECK(finishBackupGetResult(lock, &written, NULL) && written);
    CHECK(!getLockIDStatus());
    CHECK(readFlashWaitSuccess(0x25f80, restored, sizeof(restored), ReadError));
    CHECK(SDL_memcmp(source, restored, sizeof(source)) == 0);
    CHECK(!readFlashWaitSuccess(512 * 1024 - 1, restored, 2, ReadError));
    CHECK(read_errors == 1);
    PcCard_CloseBackup();
    CHECK(PcCard_OpenBackup(path));
    CHECK(readFlashWaitSuccess(0x25f80, restored, sizeof(restored), ReadError));
    CHECK(SDL_memcmp(source, restored, sizeof(source)) == 0);
    CHECK(readFlashWaitSuccess(0x25f7f, restored, 1, ReadError) && restored[0] == 0xff);
    mainBackupSystemInit(HEAPID_SYSTEM, ReadError, NULL);
    SaveData *save = SaveData_InitMain(&save_params, HEAPID_SYSTEM);
    CHECK(SaveData_Load(save, HEAPID_SYSTEM) == SAVE_LOAD_RESULT_NONE);
    SDL_memcpy(SaveData_GetBlockPtr(save, 0), source, BlockSize());
    SDL_memcpy(SaveData_GetBlockPtr(save, 1), source + 128, BlockSize());
    SaveData_SaveStart(save);
    u32 status = SAVE_RESULT_CONTINUE;
    for (unsigned i = 0; i < 10000 && status != SAVE_RESULT_OK && status != SAVE_RESULT_NG; i++)
        status = SaveData_SaveStep(save);
    CHECK(status == SAVE_RESULT_OK && !GCTX_HIDIsSleepBlocked(1));
    freeSaveBlocks(save);
    PcCard_CloseBackup();
    CHECK(PcCard_OpenBackup(path));
    save = SaveData_InitMain(&save_params, HEAPID_SYSTEM);
    CHECK(SaveData_Load(save, HEAPID_SYSTEM) == SAVE_LOAD_RESULT_OK);
    CHECK(SDL_memcmp(SaveData_GetBlockPtr(save, 0), source, BlockSize()) == 0);
    CHECK(SDL_memcmp(SaveData_GetBlockPtr(save, 1), source + 128, BlockSize()) == 0);
    // Damage one saved copy: the original CRC and backup-selection code must recover the other.
    u8 damaged = 0xff;
    CHECK(writeFlashWaitSuccess(0, &damaged, 1, NULL));
    freeSaveBlocks(save);
    save = SaveData_InitMain(&save_params, HEAPID_SYSTEM);
    status = SaveData_Load(save, HEAPID_SYSTEM);
    CHECK(status == SAVE_LOAD_RESULT_OK);
    CHECK(SDL_memcmp(SaveData_GetBlockPtr(save, 0), source, BlockSize()) == 0);
    freeSaveBlocks(save);
    CHECK(writeFlashWaitSuccess(0x1000, &damaged, 1, NULL));
    save = SaveData_InitMain(&save_params, HEAPID_SYSTEM);
    CHECK(SaveData_Load(save, HEAPID_SYSTEM) == SAVE_LOAD_RESULT_BROKEN);
    freeSaveBlocks(save);
    GFL_HeapDelete(HEAPID_SYSTEM);
    PcCard_CloseBackup();
    CHECK(SDL_RemovePath(path));
    SDL_IOStream *invalid = SDL_IOFromFile(path, "wb");
    CHECK(invalid && SDL_WriteIO(invalid, source, 3) == 3 && SDL_CloseIO(invalid));
    CHECK(!PcCard_OpenBackup(path));
    SDL_PathInfo info;
    CHECK(SDL_GetPathInfo(path, &info) && info.size == 3);
    CHECK(SDL_RemovePath(path));
#ifndef _WIN32
    // A dangling symlink occupies the save path even though following it fails.
    CHECK(symlink("missing-save-target.sav", path) == 0);
    CHECK(!PcCard_OpenBackup(path));
    CHECK(unlink(path) == 0);
#endif
    SDL_Quit();
    puts("Original backup callbacks, 64-bit buffers, persistence and save bounds passed");
    return EXIT_SUCCESS;
}
