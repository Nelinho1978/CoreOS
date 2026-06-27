#ifndef COREOS_PRINTK_H
#define COREOS_PRINTK_H

#include "coreos/types.h"

void kputc(char c);
void kputs(const char *s);
void kprintf(const char *fmt, ...);
void kprint_hex(uint32_t value);

#endif
