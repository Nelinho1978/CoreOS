#include "coreos/drivers_hw.h"
#include "coreos/printk.h"
#include "coreos/usb.h"
#include "gfx/gfx_accel.h"
#include "gfx/opengl.h"
#include "gfx/directx.h"
#include "gfx/display.h"
#include "arch/ac97.h"

extern int e1000_inicializar(void);
extern int e1000_esta_pronto(void);

static driver_status_t g_drv_status;

void drivers_inicializar_hardware(void) {
    kputs("[Drv] Inicializando hardware...\n");

    g_drv_status.video    = 1;
    g_drv_status.teclado  = 1;
    g_drv_status.mouse    = 0;

    /* GPU / aceleração */
    g_drv_status.gpu_accel = gfx_accel_init() ? 1 : 0;

    /* Display / monitor */
    g_drv_status.display = display_detect_init() ? 1 : 0;

    /* OpenGL / DirectX (software) */
    g_drv_status.opengl  = gl_esta_pronto() ? 1 : 0;
    g_drv_status.directx = d3d_esta_pronto() ? 1 : 0;

    /* USB */
    if (usb_inicializar()) {
        g_drv_status.usb = 1;
    } else {
        g_drv_status.usb = 0;
    }

    /* Rede (Intel 82540EM) */
    if (e1000_inicializar()) {
        g_drv_status.rede = 1;
        kputs("[Drv] Rede 82540EM ativo\n");
    } else {
        g_drv_status.rede = 0;
    }

    /* Som (AC97) */
    if (ac97_inicializar()) {
        g_drv_status.som = 1;
        kputs("[Drv] Som AC97 ativo\n");
    } else {
        g_drv_status.som = 0;
    }

    drivers_imprimir_relatorio();
}

void drivers_marcar_mouse(int ok) {
    g_drv_status.mouse = ok ? 1 : 0;
}

const driver_status_t *drivers_estado(void) {
    return &g_drv_status;
}

void drivers_imprimir_relatorio(void) {
    kputs("[Drv] Relatorio de drivers:\n");
    kputs("  Video    "); kputs(g_drv_status.video    ? "OK" : "FALHOU"); kputs("\n");
    kputs("  Teclado  "); kputs(g_drv_status.teclado  ? "OK" : "FALHOU"); kputs("\n");
    kputs("  Mouse    "); kputs(g_drv_status.mouse    ? "OK" : "FALHOU"); kputs("\n");
    kputs("  GPU Acc  "); kputs(g_drv_status.gpu_accel ? "OK" : "OFF");   kputs("\n");
    kputs("  Display  "); kputs(g_drv_status.display  ? "OK" : "OFF");   kputs("\n");
    kputs("  OpenGL   "); kputs(g_drv_status.opengl   ? "OK" : "OFF");   kputs("\n");
    kputs("  DirectX  "); kputs(g_drv_status.directx  ? "OK" : "OFF");   kputs("\n");
    kputs("  USB      "); kputs(g_drv_status.usb      ? "OK" : "OFF");   kputs("\n");
    kputs("  Rede     "); kputs(g_drv_status.rede     ? "OK" : "OFF");   kputs("\n");
    kputs("  Som      "); kputs(g_drv_status.som      ? "OK" : "OFF");   kputs("\n");
}
