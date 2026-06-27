#include "gfx/gfx_accel.h"
#include "gfx/opengl.h"
#include "gfx/directx.h"
#include "gfx/raster.h"
#include "arch/bochs_svga.h"
#include "coreos/firmware.h"
#include "coreos/printk.h"
#include "gui/fb.h"
#include "arch/svga.h"

static gfx_accel_state_t g_gfx;
static int g_gfx_ready;

static int pci_encontrar_display(pci_device_t *out) {
    const firmware_state_t *fw = firmware_estado();
    uint32_t i;

    if (!out) {
        return 0;
    }

    for (i = 0; i < fw->pci_count; ++i) {
        if (fw->pci[i].class_code == 0x03u) {
            *out = fw->pci[i];
            return 1;
        }
    }
    return 0;
}

static void gpu_nome_curto(const pci_device_t *gpu, char *out, size_t out_len) {
    size_t i = 0;

    if (!gpu || !out || out_len == 0) {
        return;
    }

    out[0] = '\0';

    if (gpu->vendor_id == BOCHS_VENDOR_ID) {
        if (gpu->device_id == BOCHS_DEVICE_VGA) {
            const char *s = "Bochs VBE (VBoxVGA)";
            for (i = 0; s[i] && i + 1 < out_len; ++i) {
                out[i] = s[i];
            }
        } else if (gpu->device_id == BOCHS_DEVICE_VBOX) {
            const char *s = "VirtualBox SVGA II";
            for (i = 0; s[i] && i + 1 < out_len; ++i) {
                out[i] = s[i];
            }
        } else {
            const char *s = "Bochs Display";
            for (i = 0; s[i] && i + 1 < out_len; ++i) {
                out[i] = s[i];
            }
        }
    } else if (gpu->vendor_id == 0x8086u) {
        const char *s = "Intel Graphics";
        for (i = 0; s[i] && i + 1 < out_len; ++i) {
            out[i] = s[i];
        }
    } else if (gpu->vendor_id == 0x10DEu) {
        const char *s = "NVIDIA GPU";
        for (i = 0; s[i] && i + 1 < out_len; ++i) {
            out[i] = s[i];
        }
    } else if (gpu->vendor_id == 0x1002u) {
        const char *s = "AMD GPU";
        for (i = 0; s[i] && i + 1 < out_len; ++i) {
            out[i] = s[i];
        }
    } else {
        const char *s = "Display PCI";
        for (i = 0; s[i] && i + 1 < out_len; ++i) {
            out[i] = s[i];
        }
    }

    out[out_len - 1] = '\0';
}

static void fill_line_fast(uint32_t *line, int count, uint32_t color) {
    if (count <= 0) {
        return;
    }

    if (g_gfx.sse_ok) {
        __asm__ volatile(
            "cld\n\t"
            "rep stosl"
            : "+D"(line), "+c"(count)
            : "a"(color)
            : "memory");
        return;
    }

    while (count-- > 0) {
        *line++ = color;
    }
}

static int fill_rect_cpu(int x, int y, int w, int h, uint32_t color) {
    const svga_mode_t *m = svga_get_mode();
    int x0;
    int y0;
    int x1;
    int y1;
    int row;
    uint32_t px;

    if (!m || !m->pixels || m->bpp != 32) {
        return 0;
    }

    x0 = x < 0 ? 0 : x;
    y0 = y < 0 ? 0 : y;
    x1 = x + w;
    y1 = y + h;

    if ((uint32_t)x1 > m->width) {
        x1 = (int)m->width;
    }
    if ((uint32_t)y1 > m->height) {
        y1 = (int)m->height;
    }

    w = x1 - x0;
    h = y1 - y0;
    if (w <= 0 || h <= 0) {
        return 1;
    }

    px = svga_pack_color(color);
    for (row = y0; row < y1; ++row) {
        fill_line_fast(m->pixels + row * (m->pitch / 4u) + (uint32_t)x0, w, px);
    }
    return 1;
}

int gfx_accel_init(void) {
    const firmware_state_t *fw = firmware_estado();
    pci_device_t gpu;
    int gpu_found = 0;

    if (g_gfx_ready) {
        return 1;
    }

    g_gfx.backend = GFX_BACKEND_NONE;
    g_gfx.opengl_ok = 0;
    g_gfx.directx_ok = 0;
    g_gfx.sse_ok = (fw->cpu.features_edx & (1u << 25)) != 0u;
    g_gfx.gpu.present = 0;
    g_gfx.gpu.name[0] = '\0';

    if (pci_encontrar_display(&gpu)) {
        gpu_found = 1;
        g_gfx.gpu.present = 1;
        g_gfx.gpu.vendor_id = gpu.vendor_id;
        g_gfx.gpu.device_id = gpu.device_id;
        gpu_nome_curto(&gpu, g_gfx.gpu.name, sizeof(g_gfx.gpu.name));
    }

    if (gpu_found && bochs_svga_inicializar(&gpu)) {
        g_gfx.backend = GFX_BACKEND_BOCHS_SVGA;
    } else if (g_gfx.sse_ok) {
        g_gfx.backend = GFX_BACKEND_CPU_FAST;
        kputs("[GFX] Aceleracao CPU (rep stos / SSE pipeline)\n");
    } else {
        g_gfx.backend = GFX_BACKEND_CPU_FAST;
        kputs("[GFX] Aceleracao CPU (modo basico)\n");
    }

    g_gfx.opengl_ok = gl_init();
    g_gfx.directx_ok = d3d_init();

    kputs("[GFX] OpenGL: ");
    kputs(g_gfx.opengl_ok ? "OK (software rasterizer)" : "FALHOU");
    kputs("\n[GFX] DirectX: ");
    kputs(g_gfx.directx_ok ? "OK (D3D software layer)" : "FALHOU");
    kputs("\n");

    g_gfx_ready = 1;
    return 1;
}

const gfx_accel_state_t *gfx_accel_estado(void) {
    return &g_gfx;
}

const char *gfx_accel_backend_nome(void) {
    switch (g_gfx.backend) {
    case GFX_BACKEND_BOCHS_SVGA:
        return bochs_svga_esta_pronto() ? bochs_svga_nome() : "Bochs SVGA";
    case GFX_BACKEND_CPU_FAST:
        return g_gfx.sse_ok ? "CPU rep stos" : "CPU";
    default:
        return "Nenhum";
    }
}

int gfx_accel_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (!g_gfx_ready) {
        return 0;
    }

    if (g_gfx.backend == GFX_BACKEND_BOCHS_SVGA) {
        if (bochs_svga_fill_rect(x, y, w, h, color)) {
            return 1;
        }
    }

    return fill_rect_cpu(x, y, w, h, color);
}

int gfx_accel_blit_rect(const uint32_t *src, int sw, int dx, int dy, int dw, int dh) {
    const svga_mode_t *m = svga_get_mode();
    int row;
    int col;

    if (!g_gfx_ready || !src || !m || !m->pixels || m->bpp != 32 || sw <= 0 || dw <= 0 || dh <= 0) {
        return 0;
    }

    for (row = 0; row < dh; ++row) {
        int y = dy + row;
        if (y < 0 || (uint32_t)y >= m->height) {
            continue;
        }
        for (col = 0; col < dw; ++col) {
            int x = dx + col;
            if (x < 0 || (uint32_t)x >= m->width) {
                continue;
            }
            m->pixels[y * (m->pitch / 4u) + (uint32_t)x] = svga_pack_color(src[row * sw + col]);
        }
    }
    return 1;
}
