#include "field/field_effect.h"
#include "nitro/mi.h"
#include "nnsys/g3d.h"
#include "nnsys/snd.h"
#include "pml/poke_party.h"
#include "system/vm.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char PcPointerWidth[(sizeof(void *) == 8) ? 1 : -1];
typedef char PcAddressWidth[(sizeof(uintptr_t) == sizeof(void *)) ? 1 : -1];
typedef char PcWordWidth[(sizeof(uint32_t) == 4) ? 1 : -1];
typedef char PcBoxPokemonSize[(sizeof(BoxPkm) == 136) ? 1 : -1];
typedef char PcPartyPokemonSize[(sizeof(PartyPkm) == 220) ? 1 : -1];
typedef char PcPokemonNameBlockSize[(sizeof(PkmBlockC) == 32) ? 1 : -1];
typedef char PcPokemonRibbonOffset[(offsetof(PkmBlockC, ribbons) == 24) ? 1 : -1];
typedef struct {
    u32 prefix;
    PkmBlockC block;
} PcPokemonBlockAlignment;
typedef char PcPokemonBlockFourByteAlignment[(offsetof(PcPokemonBlockAlignment, block) == 4) ? 1 : -1];
typedef char PcDoubleWordWidth[(sizeof(uint64_t) == 8) ? 1 : -1];
typedef char PcResourceHeaderSize[(sizeof(NNSG3dResFileHeader) == 16) ? 1 : -1];
typedef char PcModelInfoSize[(sizeof(NNSG3dResMdlInfo) == 44) ? 1 : -1];
typedef char PcTextureDictionaryOffset[(offsetof(NNSG3dResTex, dict) == 60) ? 1 : -1];
typedef char PcModelNodeOffset[(offsetof(NNSG3dResMdl, nodeInfo) == 64) ? 1 : -1];
typedef char PcFieldEffectWorkAlignment[(offsetof(FieldEffectTask, work) % sizeof(void *) == 0) ? 1 : -1];

static unsigned disposed;
static void DisposeSound(void *memory, u32 size, uptr first, uptr second) {
    if (size != 7 || strcmp(memory, "native") != 0 || first != UINT64_C(0xfedcba9876543210) ||
        second != UINT64_C(0x123456789abcdef0))
        abort();
    disposed++;
}

int main(void) {
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
    if ((PcMiExMemControl & REG_MI_EXMEMCNT_EP_MASK) != 0)
        return EXIT_FAILURE;
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    if ((PcMiExMemControl & REG_MI_EXMEMCNT_EP_MASK) == 0)
        return EXIT_FAILURE;
    void *heapMemory = malloc(4096);
    NNSSndHeapHandle heap = NNS_SndHeapCreate(heapMemory, 4096);
    if (!heap || NNS_SndHeapSaveState(heap) != 1)
        return EXIT_FAILURE;
    void *block = NNS_SndHeapAlloc(heap, 7, NULL, 0, 0);
    if (!block || (uintptr_t)block % 32 != 0)
        return EXIT_FAILURE;
    int level = NNS_SndHeapSaveState(heap);
    void *next = NNS_SndHeapAlloc(heap, 7, DisposeSound, UINT64_C(0xfedcba9876543210), UINT64_C(0x123456789abcdef0));
    // Disposal checks native-width callback arguments independently of the allocation's address.
    if (!next)
        return EXIT_FAILURE;
    memcpy(next, "native", 7);
    NNS_SndHeapLoadState(heap, level);
    if (disposed != 1 || NNS_SndHeapGetCurrentLevel(heap) != level)
        return EXIT_FAILURE;
    NNS_SndHeapDestroy(heap);
    free(heapMemory);
    VMInitParam param = { .workSize = 5, .stackSize = 7 };
    VM *vm = VM_Create(HEAPID_SYSTEM, &param);
    for (unsigned i = 0; i < param.workSize; i++)
        vm->work[i] = UINTPTR_MAX;
    for (unsigned i = 0; i < param.stackSize; i++)
        vm->stack[i] = UINTPTR_MAX;
    VM_Reset(vm);
    for (unsigned i = 0; i < param.workSize; i++) {
        if (vm->work[i] != 0)
            return EXIT_FAILURE;
    }
    for (unsigned i = 0; i < param.stackSize; i++) {
        if (vm->stack[i] != 0)
            return EXIT_FAILURE;
    }
    VM_Free(vm);
    int value = 42;
    uintptr_t address = (uintptr_t)&value;
    if ((int *)address != &value || *(int *)address != 42) {
        return EXIT_FAILURE;
    }
    // Exercise bits that would be lost if a host address were stored in a DS word.
    uintptr_t wide_address = (uintptr_t)UINT64_C(0x123456789abcdef0);
    if ((uint64_t)wide_address != UINT64_C(0x123456789abcdef0)) {
        return EXIT_FAILURE;
    }
    // Exercise the recovered resource lookup with native pointers and DS-sized on-disk offsets.
    struct ResourceFixture {
        NNSG3dResDict dict;
        NNSG3dResDictEntryHeader entry;
        NNSG3dResName name;
    } resource = { 0 };
    resource.dict.numEntry = 1;
    resource.dict.ofsEntry = offsetof(struct ResourceFixture, entry);
    resource.entry.sizeUnit = sizeof(resource.entry.data);
    resource.entry.ofsName = sizeof(resource.entry);
    memcpy(&resource.name, "native-resource", 15);
    NNSG3dResName missing = { 0 };
    if (NNS_G3dGetResDataByName(&resource.dict, &resource.name) != resource.entry.data ||
        NNS_G3dGetResDictIdxByName(&resource.dict, &resource.name) != 0 ||
        NNS_G3dGetResDataByName(&resource.dict, &missing) != NULL ||
        NNS_G3dGetResDictIdxByName(&resource.dict, &missing) != -1 ||
        NNS_G3dGetResDataByIdx(&resource.dict, 1) != NULL) {
        fputs("Native resource lookup failed\n", stderr);
        return EXIT_FAILURE;
    }
    printf("64-bit ABI: pointer=%zu, uintptr_t=%zu, uint32_t=%zu, long=%zu\n", sizeof(void *), sizeof(uintptr_t),
           sizeof(uint32_t), sizeof(long));
    return EXIT_SUCCESS;
}
