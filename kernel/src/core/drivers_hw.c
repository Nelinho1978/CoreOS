#include "coreos/drivers_hw.h"
#include "coreos/firmware.h"
#include "coreos/printk.h"
#include "coreos/usb.h"
#include "gfx/gfx_accel.h"
#include "gfx/display.h"
#include "arch/e1000.h"
#include "arch/ac97.h"
#include "arch/mouse.h"

static driver_status_t g_drv;

const driver_status_t *drivers_estado(void) {
    return &g_drv;
}

void drivers_imprimir_relatorio(void) {
    kputs("\n=== Relatorio de drivers ===\n");
    kputs("  Video : ");
    kputs(g_drv.video ? "OK" : "FALHOU");
    kputs("\n  Rede  : ");
    kputs(g_drv.rede ? "OK" : "FALHOU");
    kputs("\n  Som   : ");
    kputs(g_drv.som ? "OK" : "FALHOU");
    kputs("\n  Mouse : ");
    kputs(g_drv.mouse ? "OK" : "FALHOU");
    kputs("\n  USB   : ");
    kputs(g_drv.usb ? "OK" : "FALHOU");
    kputs("\n  GPU   : ");
    kputs(g_drv.gpu_accel ? "OK" : "FALHOU");
    kputs("\n  OpenGL: ");
    kputs(g_drv.opengl ? "OK" : "FALHOU");
    kputs("\n  DirectX: ");
    kputs(g_drv.directx ? "OK" : "FALHOU");
    kputs("\n  Teclado: ");
    kputs(g_drv.teclado ? "OK" : "FALHOU");
    kputs("\n============================\n");
}

void drivers_inicializar_hardware(void) {
    const firmware_state_t *fw = firmware_estado();
    const gfx_accel_state_t *gfx = gfx_accel_estado();

    g_drv.video = fw->video_ok;
    g_drv.rede = e1000_inicializar();
    g_drv.som = ac97_inicializar();
    g_drv.teclado = 1;
    g_drv.mouse = 0; /* atualizado em desktop_run apos mouse_init */
    g_drv.usb = usb_inicializar();
    g_drv.display = display_modelo()->ready;
    g_drv.gpu_accel = gfx->gpu.present && gfx->backend != GFX_BACKEND_NONE;
    g_drv.opengl = gfx->opengl_ok;
    g_drv.directx = gfx->directx_ok;

    drivers_imprimir_relatorio();
}

void drivers_marcar_mouse(int ok) {
    g_drv.mouse = ok;
}
