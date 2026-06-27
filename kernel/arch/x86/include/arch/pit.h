#ifndef COREOS_ARCH_X86_PIT_H
#define COREOS_ARCH_X86_PIT_H

#include "coreos/types.h"

void pit_init(void);
uint16_t pit_read_ch0(void);
void pit_accumulate_ms(uint16_t *last, uint32_t *elapsed_ms, uint32_t *rem_ticks);

#endif
