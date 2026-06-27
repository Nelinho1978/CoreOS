#ifndef COREOS_KERNEL_H
#define COREOS_KERNEL_H

#include "coreos/types.h"

#define COREOS_NAME "CoreOS 10"
#define COREOS_VERSION "10.0.19045"

/** Magic passado pelo boot sector proprio (sem GRUB/Multiboot) */
#define COREOS_BOOT_MAGIC_DIRECT 0xC0950095u

void kernel_main(uint32_t magic, void *boot_info);
void kernel_panic(const char *message) __attribute__((noreturn));

#endif
