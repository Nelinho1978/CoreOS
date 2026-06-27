#ifndef COREOS_BOOT_INFO_H
#define COREOS_BOOT_INFO_H

#include "coreos/types.h"

#define BOOT_INFO_ADDR 0x9000u
#define BOOT_FB_MAGIC  0xFB950095u

#define DISPLAY_INFO_ADDR  0x9020u
#define DISPLAY_INFO_MAGIC 0xD15010A1u
#define DISPLAY_BOOT_MAX_MODES 12u

typedef struct boot_framebuffer {
    uint32_t magic;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    uint32_t framebuffer_phys;
} boot_framebuffer_t;

typedef struct display_mode_boot {
    uint16_t mode_id;
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint8_t linear;
} display_mode_boot_t;

typedef struct display_boot_info {
    uint32_t magic;
    uint16_t vbe_version;
    uint16_t selected_mode;
    uint16_t monitor_max_w;
    uint16_t monitor_max_h;
    uint16_t monitor_preferred_w;
    uint16_t monitor_preferred_h;
    uint8_t mode_count;
    uint8_t scan_ok;
    uint8_t reserved[2];
    display_mode_boot_t modes[DISPLAY_BOOT_MAX_MODES];
} display_boot_info_t;

static inline boot_framebuffer_t *boot_framebuffer_get(void) {
    return (boot_framebuffer_t *)BOOT_INFO_ADDR;
}

static inline display_boot_info_t *display_boot_info_get(void) {
    return (display_boot_info_t *)DISPLAY_INFO_ADDR;
}

#endif
