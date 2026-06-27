#include "ntos/mm.h"
#include "coreos/kernel.h"
#include "coreos/memory.h"
#include "coreos/printk.h"

#define POOL_SLOTS 16u
#define POOL_SLOT_SIZE 4096u

static uint8_t g_pool[POOL_SLOTS][POOL_SLOT_SIZE];
static uint8_t g_pool_used[POOL_SLOTS];

void MmInitSystem(uint32_t boot_magic, void *boot_info) {
    ULONG i;

    for (i = 0; i < POOL_SLOTS; ++i) {
        g_pool_used[i] = 0;
    }

    if (boot_magic == COREOS_BOOT_MAGIC_DIRECT) {
        memory_init_direct();
    } else {
        memory_init(boot_info);
    }

    kputs("[Mm] Memory Manager inicializado\n");
}

const memory_map_t *MmGetPhysicalMemoryMap(void) {
    return memory_get_map();
}

PVOID MmAllocatePool(SIZE_T bytes) {
    ULONG i;

    if (bytes == 0 || bytes > POOL_SLOT_SIZE) {
        return NULL;
    }

    for (i = 0; i < POOL_SLOTS; ++i) {
        if (!g_pool_used[i]) {
            g_pool_used[i] = 1;
            return g_pool[i];
        }
    }

    return NULL;
}

void MmFreePool(PVOID ptr) {
    ULONG i;

    if (!ptr) {
        return;
    }

    for (i = 0; i < POOL_SLOTS; ++i) {
        if (ptr == g_pool[i]) {
            g_pool_used[i] = 0;
            return;
        }
    }
}
