#ifndef COREOS_MEMORY_H
#define COREOS_MEMORY_H

#include "coreos/types.h"

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
} memory_region_t;

#define MEMORY_MAX_REGIONS 64

typedef struct {
    memory_region_t regions[MEMORY_MAX_REGIONS];
    uint32_t count;
    uint64_t total_usable;
} memory_map_t;

void memory_init(void *boot_info);
void memory_init_direct(void);
void memory_set_ram_topo(uint32_t bytes);
const memory_map_t *memory_get_map(void);

#endif
