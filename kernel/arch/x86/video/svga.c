#include "arch/svga.h"
#include "coreos/boot_info.h"
#include "arch/paging.h"

static svga_mode_t g_mode;
static int g_ready;

uint32_t svga_pack_color(uint32_t rgb) {
    uint8_t r = (uint8_t)((rgb >> 16) & 0xFFu);
    uint8_t g = (uint8_t)((rgb >> 8) & 0xFFu);
    uint8_t b = (uint8_t)(rgb & 0xFFu);
    return (uint32_t)b | ((uint32_t)g << 8) | ((uint32_t)r << 16);
}

int svga_init_from_boot_info(void) {
    const boot_framebuffer_t *bi = boot_framebuffer_get();

    if (g_ready) {
        return 1;
    }

    if (bi->magic != BOOT_FB_MAGIC || bi->framebuffer_phys == 0) {
        return 0;
    }
    if (bi->bpp != 32 && bi->bpp != 8) {
        return 0;
    }
    if (bi->width < 320 || bi->height < 200) {
        return 0;
    }

    g_mode.width = bi->width;
    g_mode.height = bi->height;
    g_mode.pitch = bi->pitch;
    g_mode.bpp = (uint8_t)bi->bpp;
    g_mode.phys = bi->framebuffer_phys;
    g_mode.pixels = NULL;
    g_mode.fb8 = NULL;

    paging_init(bi->framebuffer_phys, bi->pitch * bi->height);

    if (bi->bpp == 32) {
        g_mode.pixels = (uint32_t *)(uint32_t)bi->framebuffer_phys;
    } else {
        g_mode.fb8 = (volatile uint8_t *)(uint32_t)bi->framebuffer_phys;
    }

    g_ready = 1;
    return 1;
}

const svga_mode_t *svga_get_mode(void) {
    return &g_mode;
}

int svga_is_ready(void) {
    return g_ready;
}
