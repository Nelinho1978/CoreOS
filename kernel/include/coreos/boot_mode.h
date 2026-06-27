#ifndef COREOS_BOOT_MODE_H
#define COREOS_BOOT_MODE_H

#include "coreos/types.h"

typedef enum {
    BOOT_MODE_INSTALL = 1,
    BOOT_MODE_LIVE = 2,
} boot_mode_t;

boot_mode_t boot_menu_show(void);
void setup_run_installer(void);
void kernel_run_live(uint32_t magic, void *boot_info);

#endif
