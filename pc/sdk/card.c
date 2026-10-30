#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include "nitro/card.h"
#include "gfl/std.h"
#include "nitro/host_card.h"
#include "nitro/os.h"
#include <SDL3/SDL.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

enum { BACKUP_SIZE = 512 * 1024, LOCK_COUNT = 128 };
static u8 backup[BACKUP_SIZE];
static char *backup_path;
static SDL_IOStream *backup_file;
static CARDResult result;
static bool lock_ids[LOCK_COUNT];
static SDL_SpinLock lock_guard;
static SDL_Mutex *backup_mutex;

static bool PathMissing(const char *path) {
#ifdef _WIN32
    wchar_t *wide = (wchar_t *)SDL_iconv_string("WCHAR_T", "UTF-8", path, SDL_strlen(path) + 1);
    if (!wide)
        return false;
    DWORD attributes = GetFileAttributesW(wide);
    DWORD error = GetLastError();
    SDL_free(wide);
    return attributes == INVALID_FILE_ATTRIBUTES && (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND);
#else
    struct stat info;
    return lstat(path, &info) != 0 && errno == ENOENT;
#endif
}

// Publish a complete erased flash image without replacing a save created by another process.
static BOOL CreateBackup(void) {
    char *temporary = NULL;
    int fd = -1;
    BOOL complete = FALSE;
    if (SDL_asprintf(&temporary, "%s.tmp-%llu", backup_path, (unsigned long long)SDL_GetPerformanceCounter()) < 0)
        return FALSE;
#ifdef _WIN32
    wchar_t *temporary_w = (wchar_t *)SDL_iconv_string("WCHAR_T", "UTF-8", temporary, SDL_strlen(temporary) + 1);
    wchar_t *path_w = (wchar_t *)SDL_iconv_string("WCHAR_T", "UTF-8", backup_path, SDL_strlen(backup_path) + 1);
    if (temporary_w && path_w)
        fd = _wopen(temporary_w, _O_CREAT | _O_EXCL | _O_WRONLY | _O_BINARY, _S_IREAD | _S_IWRITE);
    if (fd >= 0) {
        complete = _write(fd, backup, sizeof(backup)) == sizeof(backup);
        if (complete && _commit(fd) != 0)
            complete = FALSE;
        if (_close(fd) != 0)
            complete = FALSE;
        if (complete)
            complete = MoveFileExW(temporary_w, path_w, 0) != 0;
        if (!complete)
            DeleteFileW(temporary_w);
    }
    SDL_free(temporary_w);
    SDL_free(path_w);
#else
    fd = open(temporary, O_CREAT | O_EXCL | O_WRONLY, 0600);
    if (fd >= 0) {
        complete = write(fd, backup, sizeof(backup)) == sizeof(backup);
        if (complete && fsync(fd) != 0)
            complete = FALSE;
        if (close(fd) != 0)
            complete = FALSE;
        // link() fails if the destination exists; rename() would overwrite a concurrent creator.
        if (complete)
            complete = link(temporary, backup_path) == 0;
        unlink(temporary);
    }
#endif
    SDL_free(temporary);
    if (!complete)
        return SDL_SetError("Could not initialize save file without replacing existing data");
    backup_file = SDL_IOFromFile(backup_path, "r+b");
    return backup_file != NULL;
}

BOOL PcCard_OpenBackup(const char *path) {
    SDL_PathInfo info;
    PcCard_CloseBackup();
    SDL_memset(backup, 0xff, sizeof(backup));
    backup_path = SDL_strdup(path);
    backup_mutex = SDL_CreateMutex();
    if (!backup_path || !backup_mutex) {
        PcCard_CloseBackup();
        return FALSE;
    }
    if (SDL_GetPathInfo(path, &info)) {
        if (info.type != SDL_PATHTYPE_FILE || (info.size < BACKUP_SIZE || info.size > BACKUP_SIZE + 544)) {
            PcCard_CloseBackup();
            return SDL_SetError("Save file has an unsupported size");
        }
        backup_file = SDL_IOFromFile(path, "r+b");
        if (!backup_file || SDL_ReadIO(backup_file, backup, sizeof(backup)) != sizeof(backup)) {
            PcCard_CloseBackup();
            return FALSE;
        }
    } else if (!PathMissing(path)) {
        PcCard_CloseBackup();
        return SDL_SetError("Could not inspect existing save path: %s", path);
    }
    result = CARD_RESULT_SUCCESS;
    return TRUE;
}
void PcCard_CloseBackup(void) {
    if (backup_file)
        SDL_CloseIO(backup_file);
    if (backup_mutex)
        SDL_DestroyMutex(backup_mutex);
    SDL_free(backup_path);
    backup_path = NULL;
    backup_file = NULL;
    backup_mutex = NULL;
}
s32 OS_GetLockID(void) {
    SDL_LockSpinlock(&lock_guard);
    for (unsigned i = 0; i < LOCK_COUNT; i++) {
        if (!lock_ids[i]) {
            lock_ids[i] = true;
            SDL_UnlockSpinlock(&lock_guard);
            return (s32)i + 1;
        }
    }
    SDL_UnlockSpinlock(&lock_guard);
    return OS_LOCK_ID_ERROR;
}
void OS_ReleaseLockID(u16 id) {
    if (!id || id > LOCK_COUNT)
        GFL_DebugAssertFailEx(__FILE__, __LINE__, "Invalid native lock ID %u", id);
    SDL_LockSpinlock(&lock_guard);
    lock_ids[id - 1] = false;
    SDL_UnlockSpinlock(&lock_guard);
}
void CARD_LockBackup(u16 id) {
    (void)id;
    SDL_LockMutex(backup_mutex);
}
void CARD_UnlockBackup(u16 id) {
    (void)id;
    SDL_UnlockMutex(backup_mutex);
}
BOOL CARD_IdentifyBackup(CARDBackupType type) {
    return backup_path && type == CARD_BACKUP_TYPE_FLASH_4MBITS_EX;
}
CARDResult CARD_GetResultCode(void) {
    return result;
}
BOOL CARDi_RequestStreamCommand(uptr src, uptr dest, u32 size, MIDmaCallback callback, void *arg, BOOL async,
                                CARDRequest request, int retries, CARDRequestMode mode) {
    (void)async;
    (void)retries;
    uptr offset = request == CARD_REQ_READ_BACKUP ? src : dest;
    result = CARD_RESULT_INVALID_PARAM;
    if (backup_path && offset <= BACKUP_SIZE && size <= BACKUP_SIZE - offset) {
        if (request == CARD_REQ_READ_BACKUP && dest) {
            SDL_memcpy((void *)dest, backup + offset, size);
            result = CARD_RESULT_SUCCESS;
        } else if (request == CARD_REQ_WRITE_BACKUP && src) {
            // A short initial write must not leave a truncated save at the final path.
            if (!backup_file)
                CreateBackup();
            result = CARD_RESULT_FAILURE;
            if (backup_file && SDL_SeekIO(backup_file, (Sint64)offset, SDL_IO_SEEK_SET) >= 0 &&
                SDL_WriteIO(backup_file, (const void *)src, size) == size && SDL_FlushIO(backup_file)) {
                SDL_memcpy(backup + offset, (const void *)src, size);
                result = CARD_RESULT_SUCCESS;
                if (mode == CARD_REQUEST_MODE_SEND_VERIFY) {
                    if (SDL_SeekIO(backup_file, (Sint64)offset, SDL_IO_SEEK_SET) < 0 ||
                        SDL_ReadIO(backup_file, backup + offset, size) != size ||
                        SDL_memcmp(backup + offset, (const void *)src, size) != 0)
                        result = CARD_RESULT_FAILURE;
                }
            }
        }
    }
    // The desktop write completes synchronously; callbacks still precede the original GFL completion check.
    if (callback)
        callback(arg);
    return result == CARD_RESULT_SUCCESS;
}
BOOL CARD_WaitBackupAsync(void) {
    return result == CARD_RESULT_SUCCESS;
}
BOOL CARD_TryWaitBackupAsync(void) {
    return TRUE;
}
void CARD_CancelBackupAsync(void) {
    // No request survives CARDi_RequestStreamCommand on this backend.
}

// Alternating, checksummed records keep the previous settings if a write is interrupted.
// Flash addressing remains limited to the original 512 KiB.
static u32 SettingsChecksum(const u8 *data) {
    u32 hash = 2166136261u;
    for (unsigned i = 0; i < 268; i++)
        hash = (hash ^ data[i]) * 16777619u;
    return hash;
}
static u32 SettingsWord(const u8 *data) {
    return data[0] | (u32)data[1] << 8 | (u32)data[2] << 16 | (u32)data[3] << 24;
}
static void SettingsPutWord(u8 *data, u32 value) {
    for (unsigned i = 0; i < 4; i++)
        data[i] = (u8)(value >> (8 * i));
}
static BOOL ReadSettingsRecord(unsigned slot, u8 *record) {
    return backup_file && SDL_SeekIO(backup_file, BACKUP_SIZE + slot * 272, SDL_IO_SEEK_SET) >= 0 &&
           SDL_ReadIO(backup_file, record, 272) == 272 && SDL_memcmp(record, "BW2PCSET", 8) == 0 &&
           SettingsWord(record + 268) == SettingsChecksum(record);
}
static int LatestSettings(u8 records[2][272]) {
    BOOL first = ReadSettingsRecord(0, records[0]), second = ReadSettingsRecord(1, records[1]);
    if (!first)
        return second ? 1 : -1;
    if (!second)
        return 0;
    return (s32)(SettingsWord(records[1] + 8) - SettingsWord(records[0] + 8)) > 0 ? 1 : 0;
}
BOOL PcCard_ReadSettings(void *data, u32 size) {
    if (!backup_mutex || size != 256)
        return FALSE;
    u8 records[2][272];
    SDL_LockMutex(backup_mutex);
    int slot = LatestSettings(records);
    if (slot >= 0)
        SDL_memcpy(data, records[slot] + 12, size);
    SDL_UnlockMutex(backup_mutex);
    return slot >= 0;
}
BOOL PcCard_WriteSettings(const void *data, u32 size) {
    if (!backup_mutex || size != 256)
        return SDL_SetError("Save is not open");
    u8 records[2][272], record[272] = { 0 };
    SDL_LockMutex(backup_mutex);
    BOOL ok = backup_file != NULL || CreateBackup();
    int previous = LatestSettings(records);
    unsigned slot = previous == 0 ? 1 : 0;
    SDL_memcpy(record, "BW2PCSET", 8);
    SettingsPutWord(record + 8, previous < 0 ? 1 : SettingsWord(records[previous] + 8) + 1);
    SDL_memcpy(record + 12, data, size);
    SettingsPutWord(record + 268, SettingsChecksum(record));
    if (ok)
        ok = SDL_SeekIO(backup_file, BACKUP_SIZE + slot * 272, SDL_IO_SEEK_SET) >= 0 &&
             SDL_WriteIO(backup_file, record, sizeof(record)) == sizeof(record) && SDL_FlushIO(backup_file);
    SDL_UnlockMutex(backup_mutex);
    return ok;
}
