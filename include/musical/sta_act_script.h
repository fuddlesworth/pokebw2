#ifndef POKEBW2_APP_MUSICAL_STA_ACT_SCRIPT_H
#define POKEBW2_APP_MUSICAL_STA_ACT_SCRIPT_H

// Overlay 209's sta_act_script.c: runs the program's scripts, up to 10 at once, each in a VM with the commands of
// script_command.c, and the tasks the commands start

#include "types.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nnsys/fnd.h"
#include "struct_decls.h"
#include "system/vm.h"

#define STA_SCRIPT_MAX 10

// StaActScript.flags
#define STA_SCRIPT_FLAG_END 0x1
// Runs in step with the other synced scripts, waiting for them at STA_SCRIPT_FLAG_WAIT_SYNC
#define STA_SCRIPT_FLAG_SYNC 0x2
#define STA_SCRIPT_FLAG_WAIT_SYNC 0x4
// The script owns its file, which is freed with it
#define STA_SCRIPT_FLAG_OWN_FILE 0x8

typedef struct {
    StaActScriptSys *sys;
    VM *vm;
    const void *data;
    void *file;
    // Frames the script has run
    u32 frame;
    // Frames until the script runs again
    u16 wait;
    u16 flags;
    // The Pokémon the script moves
    u8 pokeNo;
    u8 index;
} StaActScript;

// A task a script command started, and its work, which is freed with it
typedef struct {
    NNSFndLink link;
    void *work;
    TCB *tcb;
} StaActScriptTask;

struct StaActScriptSys {
    HeapID heapId;
    TCBManager *tcbMgr;
    void *tcbWork;
    StaActScript *scripts[STA_SCRIPT_MAX];
    StaActing *stage;
    NNSFndList tasks;
};

StaActScriptSys *StaActScript_InitSystem(HeapID heapId, StaActing *stage);
void StaActScript_TermSystem(StaActScriptSys *sys);
void StaActScript_UpdateSystem(StaActScriptSys *sys);
StaActScript *StaActScript_CreateScript(StaActScriptSys *sys, const void *data, BOOL sync);
// Runs the script at an index of a file of scripts, a table of their offsets, which the script frees
void StaActScript_CreateScriptFile(StaActScriptSys *sys, u32 *file, u32 index, u8 pokeNo);
u8 StaActScript_GetScriptNum(StaActScriptSys *sys);
StaActScriptTask *StaActScript_AddTask(StaActScriptSys *sys, TCBFunc func, void *work, u32 priority);
void StaActScript_DelTask(StaActScriptSys *sys, StaActScriptTask *task);

// script_command.c's commands
extern VMCommand STA_SCRIPT_COMMANDS[];
#define STA_SCRIPT_COMMAND_COUNT 54

#endif // POKEBW2_APP_MUSICAL_STA_ACT_SCRIPT_H
