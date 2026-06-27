#include "ntos/mm.h"
#include "coreos/kernel.h"
#include "coreos/memory.h"
#include "coreos/printk.h"

/* Small static pool for tiny allocations (< 4KB) */
#define POOL_SLOTS 64u
#define POOL_SLOT_SIZE 4096u
static uint8_t g_pool[POOL_SLOTS][POOL_SLOT_SIZE];
static uint8_t g_pool_used[POOL_SLOTS];

/* Fallback to VM allocator for larger allocations */
extern void *vm_kernel_alloc(uint32_t size);
extern void vm_kernel_free(void *ptr);

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

    kputs("[Mm] Memory Manager inicializado (64 slots x 4KB = 256KB pool)\n");
}

const memory_map_t *MmGetPhysicalMemoryMap(void) {
    return memory_get_map();
}

PVOID MmAllocatePool(SIZE_T bytes) {
    ULONG i;

    if (bytes == 0) {
        return NULL;
    }

    /* Small allocations: use static pool */
    if (bytes <= POOL_SLOT_SIZE) {
        for (i = 0; i < POOL_SLOTS; ++i) {
            if (!g_pool_used[i]) {
                g_pool_used[i] = 1;
                return g_pool[i];
            }
        }
    }

    /* Larger allocations: use VM page allocator */
    return vm_kernel_alloc(bytes);
}

void MmFreePool(PVOID ptr) {
    ULONG i;

    if (!ptr) {
        return;
    }

    /* Check static pool first */
    for (i = 0; i < POOL_SLOTS; ++i) {
        if (ptr == g_pool[i]) {
            g_pool_used[i] = 0;
            return;
        }
    }

    /* Fallback to VM free */
    vm_kernel_free(ptr);
}
