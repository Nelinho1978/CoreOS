#ifndef COREOS_ARCH_X86_PAGING_H
#define COREOS_ARCH_X86_PAGING_H

#include "coreos/types.h"

void paging_init(uint32_t fb_phys, uint32_t fb_bytes);
void paging_map_mmio(uint32_t phys, uint32_t bytes);

#endif
