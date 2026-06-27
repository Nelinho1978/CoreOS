#include "gui/win10.h"
#include "gui/fb.h"
#include "coreos/drivers_hw.h"

static const char *const BOOT_LINES[] = {
    "Starting Windows 10...",
    "EXPLORER.EXE",
};

static void linha_status(int y, const char *nome, int ok) {
    fb_draw_string(40, y, nome, W10_FACE, W10_DESKTOP);
    fb_draw_string(160, y, ok ? "OK" : "FALHOU", ok ? W10_WHITE : 0x00FF0000u, W10_DESKTOP);
}

void boot_gfx_show(void) {
    const framebuffer_t *fb = fb_get();
    const driver_status_t *drv = drivers_estado();
    uint32_t i;
    int bar_w;

    fb_fill_rect(0, 0, (int)fb->width, (int)fb->height, W10_DESKTOP);

    for (i = 0; i < 2; ++i) {
        fb_draw_string(24, 24 + (int)i * 14, BOOT_LINES[i], W10_FACE, W10_DESKTOP);
    }

    fb_draw_string(40, 80, "Drivers (open-source / emulados):", W10_WHITE, W10_DESKTOP);
    linha_status(100, "Video SVGA/VBE", drv->video);
    linha_status(116, "GPU  Aceleracao", drv->gpu_accel);
    linha_status(132, "Monitor/GPU", drv->display);
    linha_status(148, "OpenGL SW", drv->opengl);
    linha_status(164, "DirectX SW", drv->directx);
    linha_status(180, "USB  OHCI/UHCI", drv->usb);
    linha_status(196, "Rede 82540EM", drv->rede);
    linha_status(212, "Som  AC97", drv->som);
    linha_status(228, "Teclado PS/2", drv->teclado);
    linha_status(244, "Mouse USB/PS2", drv->mouse);

    bar_w = (int)fb->width - 120;
    fb_fill_rect(60, (int)fb->height - 48, bar_w, 12, W10_SHADOW_DARK);
    fb_fill_rect(62, (int)fb->height - 46, bar_w - 4, 8, W10_TITLE_A);

    fb_draw_string((int)fb->width / 2 - 40, (int)fb->height / 2 - 8, "CoreOS 10", W10_WHITE, W10_DESKTOP);
    fb_draw_string((int)fb->width / 2 - 72, (int)fb->height / 2 + 12, "Iniciando desktop...", W10_FACE, W10_DESKTOP);

    for (i = 0; i < 150000; ++i) {
        __asm__ volatile("pause");
    }
}
