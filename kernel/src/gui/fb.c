#include "gui/fb.h"
#include "gfx/gfx_accel.h"
#include "coreos/boot_info.h"
#include "arch/svga.h"
#include "arch/ports.h"

static framebuffer_t g_fb;
static uint8_t g_fb_bpp;
static int g_fb_ready;

extern const uint8_t font8x16_basic[128][16];

static void vga_write_dac(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(0x3C8, index);
    outb(0x3C9, r);
    outb(0x3C9, g);
    outb(0x3C9, b);
}

static void vga_set_palette(void) {
    vga_write_dac(0, 0, 0, 0);
    vga_write_dac(1, 0, 48, 120);
    vga_write_dac(7, 42, 42, 42);
    vga_write_dac(8, 21, 21, 21);
    vga_write_dac(9, 0, 33, 52);
    vga_write_dac(11, 50, 55, 55);
    vga_write_dac(15, 63, 63, 63);
}

static void fb_bind_mode(const svga_mode_t *m) {
    g_fb.width = m->width;
    g_fb.height = m->height;
    g_fb.pitch = m->pitch;
    g_fb.pixels = m->pixels;
    g_fb_bpp = m->bpp;
    g_fb_ready = 1;
}

int fb_init(void) {
    const svga_mode_t *m;

    if (g_fb_ready) {
        return 1;
    }

    if (!svga_init_from_boot_info()) {
        return 0;
    }

    m = svga_get_mode();
    if (m->bpp == 8 && m->fb8) {
        vga_set_palette();
    }

    fb_bind_mode(m);

    if (m->bpp == 32 && m->pixels) {
        uint32_t row;
        uint32_t col;
        uint32_t px = svga_pack_color(0x00808080u);
        for (row = 0; row < m->height; ++row) {
            uint32_t *line = m->pixels + row * (m->pitch / 4u);
            for (col = 0; col < m->width; ++col) {
                line[col] = px;
            }
        }
    }

    return 1;
}

const framebuffer_t *fb_get(void) {
    return &g_fb;
}

uint8_t fb_bpp(void) {
    return g_fb_bpp;
}

static uint32_t fb_pixel_out(uint32_t color) {
    if (g_fb_bpp == 32) {
        return svga_pack_color(color);
    }
    return color;
}

static uint8_t fb_color_to_pixel(uint32_t color) {
    if (color <= 15u) {
        return (uint8_t)color;
    }
    if (color == 0x00808080u) {
        return 3u;
    }
    if (color == 0x0078D7u) {
        return 1u;
    }
    if (color == 0xC0C0C0u) {
        return 7u;
    }
    if (color == 0xFFFFFFu) {
        return 15u;
    }
    if (color == 0x000000u) {
        return 0u;
    }
    return (uint8_t)(color & 0xFFu);
}

void fb_put_pixel(int x, int y, uint32_t color) {
    const svga_mode_t *m = svga_get_mode();

    if (!g_fb_ready || x < 0 || y < 0 || (uint32_t)x >= g_fb.width || (uint32_t)y >= g_fb.height) {
        return;
    }

    if (g_fb_bpp == 32 && m->pixels) {
        m->pixels[y * (m->pitch / 4u) + (uint32_t)x] = fb_pixel_out(color);
        return;
    }

    if (m->fb8) {
        m->fb8[y * m->pitch + (uint32_t)x] = fb_color_to_pixel(color);
    }
}

uint32_t fb_get_pixel(int x, int y) {
    const svga_mode_t *m = svga_get_mode();

    if (!g_fb_ready || x < 0 || y < 0 || (uint32_t)x >= g_fb.width || (uint32_t)y >= g_fb.height) {
        return 0;
    }

    if (g_fb_bpp == 32 && m->pixels) {
        return m->pixels[y * (m->pitch / 4u) + (uint32_t)x];
    }

    if (m->fb8) {
        return (uint32_t)m->fb8[y * m->pitch + (uint32_t)x];
    }

    return 0;
}

void fb_fill_rect(int x, int y, int w, int h, uint32_t color) {
    const svga_mode_t *m = svga_get_mode();
    int row;
    int col;

    if (!g_fb_ready || w <= 0 || h <= 0) {
        return;
    }

    if (g_fb_bpp == 32 && m->pixels && gfx_accel_fill_rect(x, y, w, h, color)) {
        return;
    }

    if (g_fb_bpp == 32 && m->pixels) {
        uint32_t px = fb_pixel_out(color);
        int x0 = x < 0 ? 0 : x;
        int y0 = y < 0 ? 0 : y;
        int x1 = x + w;
        int y1 = y + h;

        if ((uint32_t)x1 > g_fb.width) {
            x1 = (int)g_fb.width;
        }
        if ((uint32_t)y1 > g_fb.height) {
            y1 = (int)g_fb.height;
        }

        for (row = y0; row < y1; ++row) {
            uint32_t *line = m->pixels + row * (m->pitch / 4u) + (uint32_t)x0;
            for (col = x0; col < x1; ++col) {
                *line++ = px;
            }
        }
        return;
    }

    for (row = 0; row < h; ++row) {
        for (col = 0; col < w; ++col) {
            fb_put_pixel(x + col, y + row, color);
        }
    }
}

void fb_hline(int x, int y, int w, uint32_t color) {
    int i;
    for (i = 0; i < w; ++i) {
        fb_put_pixel(x + i, y, color);
    }
}

void fb_vline(int x, int y, int h, uint32_t color) {
    int i;
    for (i = 0; i < h; ++i) {
        fb_put_pixel(x, y + i, color);
    }
}

void fb_draw_rect(int x, int y, int w, int h, uint32_t color) {
    fb_hline(x, y, w, color);
    fb_hline(x, y + h - 1, w, color);
    fb_vline(x, y, h, color);
    fb_vline(x + w - 1, y, h, color);
}

void fb_blit_char(int x, int y, char c, uint32_t fg, uint32_t bg) {
    const uint8_t *glyph;
    int row;
    int col;
    unsigned char ch = (unsigned char)c;

    if (ch < 32 || ch > 127) {
        ch = '?';
    }

    glyph = font8x16_basic[ch - 32u];
    for (row = 0; row < 8; ++row) {
        uint8_t bits = glyph[row];
        for (col = 0; col < 8; ++col) {
            fb_put_pixel(x + col, y + row, (bits & (0x80 >> col)) ? fg : bg);
        }
    }
}

void fb_draw_string(int x, int y, const char *text, uint32_t fg, uint32_t bg) {
    int cx = x;
    if (!text) {
        return;
    }
    while (*text) {
        if (*text == '\n') {
            y += 10;
            cx = x;
        } else {
            fb_blit_char(cx, y, *text, fg, bg);
            cx += 8;
        }
        ++text;
    }
}

void fb_draw_string_transparent(int x, int y, const char *text, uint32_t fg) {
    int cx = x;
    unsigned char ch;
    const uint8_t *glyph;
    int row;
    int col;

    if (!text) {
        return;
    }

    while (*text) {
        ch = (unsigned char)*text;
        if (ch < 32 || ch > 127) {
            ++text;
            continue;
        }
        glyph = font8x16_basic[ch - 32u];
        for (row = 0; row < 8; ++row) {
            uint8_t bits = glyph[row];
            for (col = 0; col < 8; ++col) {
                if (bits & (0x80 >> col)) {
                    fb_put_pixel(cx + col, y + row, fg);
                }
            }
        }
        cx += 8;
        ++text;
    }
}
