#ifndef COREOS_NTOS_MM_H
#define COREOS_NTOS_MM_H

#include "nt/ntdef.h"
#include "coreos/memory.h"

void MmInitSystem(uint32_t boot_magic, void *boot_info);
const memory_map_t *MmGetPhysicalMemoryMap(void);
PVOID MmAllocatePool(SIZE_T bytes);
void MmFreePool(PVOID ptr);

#endif
