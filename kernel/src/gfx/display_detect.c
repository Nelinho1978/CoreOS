#include "gfx/display.h"
#include "coreos/boot_info.h"
#include "coreos/firmware.h"
#include "coreos/printk.h"
#include "arch/bochs_svga.h"
#include "arch/svga.h"
#include "gui/fb.h"

static display_model_t g_display;
static int g_display_ready;

static int pci_encontrar_gpu(pci_device_t *out) {
    const firmware_state_t *fw = firmware_estado();
    uint32_t i;
    int found = 0;

    if (!out) {
        return 0;
    }

    for (i = 0; i < fw->pci_count; ++i) {
        if (fw->pci[i].class_code != 0x03u) {
            continue;
        }
        if (!found || fw->pci[i].subclass == 0x00u) {
            *out = fw->pci[i];
            found = 1;
        }
    }
    return found;
}

static void uint_to_str(uint32_t value, char *buf, size_t len) {
    char tmp[12];
    size_t n = 0;
    size_t i;

    if (!buf || len == 0) {
        return;
    }

    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    while (value > 0 && n < sizeof(tmp)) {
        tmp[n++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    for (i = 0; i < n && i + 1 < len; ++i) {
        buf[i] = tmp[n - 1u - i];
    }
    buf[i] = '\0';
}

static void str_append(char *buf, size_t len, const char *s) {
    size_t pos = 0;

    if (!buf || !s || len == 0) {
        return;
    }

    while (buf[pos] && pos + 1 < len) {
        ++pos;
    }

    while (*s && pos + 1 < len) {
        buf[pos++] = *s++;
    }
    buf[pos] = '\0';
}

static void carregar_modos_boot(const display_boot_info_t *bi) {
    uint32_t i;

    g_display.monitor.mode_count = 0;
    if (!bi || bi->magic != DISPLAY_INFO_MAGIC || bi->scan_ok == 0) {
        return;
    }

    g_display.monitor.vbe_version = bi->vbe_version;
    g_display.monitor.selected_mode_id = bi->selected_mode;
    g_display.monitor.max_width = bi->monitor_max_w;
    g_display.monitor.max_height = bi->monitor_max_h;
    g_display.monitor.preferred_width = bi->monitor_preferred_w;
    g_display.monitor.preferred_height = bi->monitor_preferred_h;

    for (i = 0; i < bi->mode_count && i < DISPLAY_MAX_MODES; ++i) {
        g_display.modes[i].mode_id = bi->modes[i].mode_id;
        g_display.modes[i].width = bi->modes[i].width;
        g_display.modes[i].height = bi->modes[i].height;
        g_display.modes[i].bpp = bi->modes[i].bpp;
        g_display.modes[i].linear = bi->modes[i].linear;
        ++g_display.monitor.mode_count;
    }
}

static void inferir_modo_kernel(void) {
    const boot_framebuffer_t *fb = boot_framebuffer_get();
    display_mode_info_t *slot;

    if (g_display.monitor.mode_count >= DISPLAY_MAX_MODES) {
        return;
    }

    slot = &g_display.modes[g_display.monitor.mode_count];
    slot->mode_id = g_display.monitor.selected_mode_id;
    slot->width = (uint16_t)fb->width;
    slot->height = (uint16_t)fb->height;
    slot->bpp = (uint8_t)fb->bpp;
    slot->linear = 1;
    ++g_display.monitor.mode_count;
}

static void detectar_gpu_pci(const pci_device_t *gpu) {
    bochs_dispi_info_t bochs;

    gpu_pci_nome(gpu, g_display.gpu.name, sizeof(g_display.gpu.name));
    gpu_pci_fabricante(gpu->vendor_id, g_display.gpu.vendor, sizeof(g_display.gpu.vendor));
    gpu_pci_classe(gpu, g_display.gpu.chip_class, sizeof(g_display.gpu.chip_class));

    g_display.gpu.vendor_id = gpu->vendor_id;
    g_display.gpu.device_id = gpu->device_id;
    g_display.gpu.bus = gpu->bus;
    g_display.gpu.device = gpu->device;
    g_display.gpu.function = gpu->function;
    g_display.gpu.subclass = gpu->subclass;
    g_display.gpu.active = 1;

    if (gpu->vendor_id == BOCHS_VENDOR_ID && bochs_dispi_ler_info(&bochs)) {
        g_display.gpu.bochs_vbe_id = bochs.vbe_id;
        g_display.gpu.vram_kb = bochs.vram_kb;

        if (bochs.xres > g_display.monitor.max_width) {
            g_display.monitor.max_width = bochs.xres;
        }
        if (bochs.yres > g_display.monitor.max_height) {
            g_display.monitor.max_height = bochs.yres;
        }
    } else {
        g_display.gpu.vram_kb = 65536u;
    }
}

static void detectar_monitor_nome(void) {
    char res[16];

    if (g_display.monitor.max_width >= 1920u && g_display.monitor.max_height >= 1080u) {
        str_append(g_display.monitor.name, sizeof(g_display.monitor.name), "Monitor Full HD");
    } else if (g_display.monitor.max_width >= 1280u) {
        str_append(g_display.monitor.name, sizeof(g_display.monitor.name), "Monitor HD");
    } else if (g_display.monitor.max_width >= 800u) {
        str_append(g_display.monitor.name, sizeof(g_display.monitor.name), "Monitor SVGA");
    } else {
        str_append(g_display.monitor.name, sizeof(g_display.monitor.name), "Monitor VGA");
    }

    str_append(g_display.monitor.name, sizeof(g_display.monitor.name), " (max ");
    uint_to_str(g_display.monitor.max_width, res, sizeof(res));
    str_append(g_display.monitor.name, sizeof(g_display.monitor.name), res);
    str_append(g_display.monitor.name, sizeof(g_display.monitor.name), "x");
    uint_to_str(g_display.monitor.max_height, res, sizeof(res));
    str_append(g_display.monitor.name, sizeof(g_display.monitor.name), res);
    str_append(g_display.monitor.name, sizeof(g_display.monitor.name), ")");
}

int display_detect_init(void) {
    const boot_framebuffer_t *fb = boot_framebuffer_get();
    const display_boot_info_t *bi = display_boot_info_get();
    pci_device_t gpu;

    if (g_display_ready) {
        return 1;
    }

    g_display.gpu.name[0] = '\0';
    g_display.gpu.vendor[0] = '\0';
    g_display.gpu.chip_class[0] = '\0';
    g_display.gpu.active = 0;
    g_display.monitor.name[0] = '\0';
    g_display.monitor.mode_count = 0;
    g_display.monitor.preferred_width = 800;
    g_display.monitor.preferred_height = 600;

    if (fb->magic != BOOT_FB_MAGIC) {
        kputs("[DISPLAY] Framebuffer boot ausente\n");
        return 0;
    }

    g_display.monitor.current_width = fb->width;
    g_display.monitor.current_height = fb->height;
    g_display.monitor.current_bpp = (uint8_t)fb->bpp;
    g_display.monitor.max_width = fb->width;
    g_display.monitor.max_height = fb->height;

    carregar_modos_boot(bi);

    if (pci_encontrar_gpu(&gpu)) {
        detectar_gpu_pci(&gpu);
    } else {
        str_append(g_display.gpu.name, sizeof(g_display.gpu.name), "Framebuffer Generico");
        str_append(g_display.gpu.vendor, sizeof(g_display.gpu.vendor), "Firmware VBE");
        str_append(g_display.gpu.chip_class, sizeof(g_display.gpu.chip_class), "VGA/VBE");
        g_display.gpu.active = 1;
    }

    if (g_display.monitor.mode_count == 0) {
        inferir_modo_kernel();
    }

    if (g_display.monitor.max_width < g_display.monitor.current_width) {
        g_display.monitor.max_width = g_display.monitor.current_width;
    }
    if (g_display.monitor.max_height < g_display.monitor.current_height) {
        g_display.monitor.max_height = g_display.monitor.current_height;
    }

    detectar_monitor_nome();
    g_display.ready = 1;
    g_display_ready = 1;

    kputs("[DISPLAY] Placa de video: ");
    kputs(g_display.gpu.name);
    kputs(" [");
    kputs(g_display.gpu.chip_class);
    kputs("]\n");
    kputs("[DISPLAY] Monitor: ");
    kputs(g_display.monitor.name);
    kputs("\n");
    kputs("[DISPLAY] Resolucao atual: ");
    kprintf("%u", g_display.monitor.current_width);
    kputs("x");
    kprintf("%u", g_display.monitor.current_height);
    kputs("x");
    kprintf("%u", (uint32_t)g_display.monitor.current_bpp);
    kputs(" — modos VBE=");
    kprintf("%u", (uint32_t)g_display.monitor.mode_count);
    kputs("\n");

    return 1;
}

const display_model_t *display_modelo(void) {
    return &g_display;
}

const gpu_adapter_info_t *display_gpu(void) {
    return &g_display.gpu;
}

const monitor_info_t *display_monitor(void) {
    return &g_display.monitor;
}

const char *display_gpu_nome(void) {
    return g_display.gpu.name;
}

const char *display_monitor_nome(void) {
    return g_display.monitor.name;
}

uint32_t display_modo_count(void) {
    return g_display.monitor.mode_count;
}

const display_mode_info_t *display_modo(uint32_t index) {
    if (index >= g_display.monitor.mode_count) {
        return NULL;
    }
    return &g_display.modes[index];
}

void display_resolucao_atual(uint32_t *w, uint32_t *h, uint8_t *bpp) {
    if (w) {
        *w = g_display.monitor.current_width;
    }
    if (h) {
        *h = g_display.monitor.current_height;
    }
    if (bpp) {
        *bpp = g_display.monitor.current_bpp;
    }
}

void display_format_resolucao(char *buf, size_t len, uint32_t w, uint32_t h, uint8_t bpp) {
    char part[12];

    if (!buf || len == 0) {
        return;
    }

    buf[0] = '\0';
    uint_to_str(w, part, sizeof(part));
    str_append(buf, len, part);
    str_append(buf, len, "x");
    uint_to_str(h, part, sizeof(part));
    str_append(buf, len, part);
    if (bpp > 0) {
        str_append(buf, len, "x");
        uint_to_str(bpp, part, sizeof(part));
        str_append(buf, len, part);
    }
}

int display_modo_contem(uint32_t width, uint32_t height, uint8_t bpp) {
    uint32_t i;

    for (i = 0; i < g_display.monitor.mode_count; ++i) {
        if (g_display.modes[i].width == width
            && g_display.modes[i].height == height
            && (bpp == 0 || g_display.modes[i].bpp == bpp)) {
            return 1;
        }
    }
    return 0;
}
