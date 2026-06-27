#include "coreos/firmware.h"
#include "coreos/memory.h"
#include "coreos/boot_info.h"
#include "coreos/printk.h"
#include "arch/cpuid.h"
#include "arch/pci.h"
#include "arch/ram_probe.h"
#include "arch/svga.h"
#include "gui/fb.h"
#include "gfx/gfx_accel.h"
#include "gfx/display.h"

static firmware_state_t g_fw;

const firmware_state_t *firmware_estado(void) {
    return &g_fw;
}

void inicializar_cpu(void) {
    cpuid_ler_info(&g_fw.cpu);
    kputs("[CPU] Fabricante: ");
    kputs(g_fw.cpu.vendor);
    kputs("  Familia=");
    kprintf("%u", g_fw.cpu.family);
    kputs(" Modelo=");
    kprintf("%u", g_fw.cpu.model);
    kputs("\n");

    if (g_fw.cpu.features_edx & (1u << 25)) {
        kputs("[CPU] Feature: SSE\n");
    }
    if (g_fw.cpu.features_ecx & (1u << 1)) {
        kputs("[CPU] Feature: AVX (indicador)\n");
    }
    if (g_fw.cpu.features_edx & (1u << 5)) {
        kputs("[CPU] Feature: MSR\n");
    }
}

void inicializar_memoria(void) {
    uint32_t topo;

    topo = ram_sondar_limite();
    if (topo < 0x00200000u) {
        topo = 32u * 1024u * 1024u;
    }

    g_fw.ram_bytes = topo;
    memory_set_ram_topo(topo);
    memory_init_direct();

    kputs("[MEM] RAM detectada ate ");
    kprint_hex(topo);
    kputs(" bytes\n");
    kputs("[MEM] Mapa: 0x00000000 - ");
    kprint_hex(topo - 1u);
    kputs(" utilizavel\n");
}

void inicializar_pci(void) {
    uint32_t i;

    pci_enumerar(g_fw.pci, FIRMWARE_MAX_PCI, &g_fw.pci_count);
    kputs("[PCI] Dispositivos encontrados: ");
    kprintf("%u", g_fw.pci_count);
    kputs("\n");

    for (i = 0; i < g_fw.pci_count; ++i) {
        const pci_device_t *d = &g_fw.pci[i];
        kputs("  [");
        kprintf("%u", (uint32_t)d->bus);
        kputc(':');
        kprintf("%u", (uint32_t)d->device);
        kputc('.');
        kprintf("%u", (uint32_t)d->function);
        kputs("] VID=");
        kprint_hex(d->vendor_id);
        kputs(" DID=");
        kprint_hex(d->device_id);
        kputs(" Classe=");
        kprint_hex(d->class_code);
        kputs("\n");
    }
}

void inicializar_video(void) {
    const boot_framebuffer_t *bi = boot_framebuffer_get();
    const svga_mode_t *modo;

    g_fw.video_ok = 0;

    if (bi->magic != BOOT_FB_MAGIC || bi->framebuffer_phys == 0) {
        kputs("[VIDEO] Framebuffer do firmware ausente\n");
        return;
    }

    if (!svga_init_from_boot_info()) {
        kputs("[VIDEO] Falha ao iniciar driver SVGA/VBE\n");
        return;
    }

    if (!fb_init()) {
        kputs("[VIDEO] Falha ao configurar framebuffer\n");
        return;
    }

    gfx_accel_init();
    display_detect_init();

    modo = svga_get_mode();
    g_fw.video_ok = 1;
    g_fw.fb_phys = bi->framebuffer_phys;
    g_fw.fb_width = bi->width;
    g_fw.fb_height = bi->height;

    kputs("[VIDEO] Modo ");
    kprintf("%u", bi->width);
    kputs("x");
    kprintf("%u", bi->height);
    kputs(" @ ");
    kprint_hex(bi->framebuffer_phys);
    kputs("\n");

    if (modo->bpp == 32 && modo->pixels) {
        modo->pixels[0] = svga_pack_color(0x00FF0000u);
        kputs("[VIDEO] Primeiro pixel = vermelho (teste)\n");
    } else if (modo->fb8) {
        modo->fb8[0] = 4u;
    }
}

void firmware_iniciar(void) {
    kputs("\n=== Firmware CoreOS — descoberta de hardware ===\n");
    inicializar_cpu();
    inicializar_memoria();
    inicializar_pci();
    inicializar_video();
    kputs("=== Firmware concluido — transferindo ao kernel ===\n\n");
}
