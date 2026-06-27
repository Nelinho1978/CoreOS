#include "coreos/memory.h"
#include "coreos/printk.h"
#include "hal/multiboot.h"

static memory_map_t g_map;
static uint32_t g_ram_topo;

void memory_set_ram_topo(uint32_t bytes) {
    g_ram_topo = bytes;
}

void memory_init(void *boot_info) {
    struct multiboot_info *info = (struct multiboot_info *)boot_info;
    g_map.count = 0;
    g_map.total_usable = 0;

    if (!(info->flags & (1 << 6))) {
        kputs("[mem] mapa de memoria indisponivel\n");
        return;
    }

    for (uint32_t addr = info->mmap_addr;
         addr < info->mmap_addr + info->mmap_length && g_map.count < MEMORY_MAX_REGIONS;) {
        struct multiboot_mmap_entry *entry = (struct multiboot_mmap_entry *)addr;

        memory_region_t *region = &g_map.regions[g_map.count++];
        region->base = entry->addr;
        region->length = entry->len;
        region->type = entry->type;

        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            g_map.total_usable += entry->len;
        }

        addr += entry->size + sizeof(entry->size);
    }
}

void memory_init_direct(void) {
    uint32_t topo = g_ram_topo;
    if (topo < 0x00200000u) {
        topo = 32u * 1024u * 1024u;
    }

    g_map.count = 1;
    g_map.total_usable = topo - 0x100000u;
    g_map.regions[0].base = 0x100000;
    g_map.regions[0].length = g_map.total_usable;
    g_map.regions[0].type = MULTIBOOT_MEMORY_AVAILABLE;
}

const memory_map_t *memory_get_map(void) {
    return &g_map;
}
