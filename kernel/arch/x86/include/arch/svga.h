#ifndef COREOS_ARCH_X86_SVGA_H
#define COREOS_ARCH_X86_SVGA_H

#include "coreos/types.h"

/* Driver SVGA/VBE — compativel com VBoxVGA e VMSVGA do VirtualBox.
 * Nao e driver NVIDIA proprietario (exige Windows/Linux + PCI). */

typedef struct svga_mode {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
    uint32_t phys;
    uint32_t *pixels;
    volatile uint8_t *fb8;
} svga_mode_t;

uint32_t svga_pack_color(uint32_t rgb);
int svga_init_from_boot_info(void);
const svga_mode_t *svga_get_mode(void);
int svga_is_ready(void);

#endif
