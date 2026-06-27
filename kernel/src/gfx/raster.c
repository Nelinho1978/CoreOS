#include "gfx/raster.h"
#include "gui/fb.h"
#include "arch/svga.h"

static uint32_t raster_pack(uint32_t rgb) {
    return svga_pack_color(rgb);
}

static int raster_clip(int *x, int *y, int *w, int *h) {
    const framebuffer_t *fb = fb_get();
    int x0;
    int y0;
    int x1;
    int y1;

    if (!fb || *w <= 0 || *h <= 0) {
        return 0;
    }

    x0 = *x < 0 ? 0 : *x;
    y0 = *y < 0 ? 0 : *y;
    x1 = *x + *w;
    y1 = *y + *h;

    if ((uint32_t)x1 > fb->width) {
        x1 = (int)fb->width;
    }
    if ((uint32_t)y1 > fb->height) {
        y1 = (int)fb->height;
    }

    *w = x1 - x0;
    *h = y1 - y0;
    if (*w <= 0 || *h <= 0) {
        return 0;
    }

    *x = x0;
    *y = y0;
    return 1;
}

void raster_fill_rect(int x, int y, int w, int h, uint32_t color) {
    const svga_mode_t *m = svga_get_mode();
    uint32_t px;
    int row;
    int col;

    if (!m || !m->pixels || m->bpp != 32) {
        fb_fill_rect(x, y, w, h, color);
        return;
    }

    if (!raster_clip(&x, &y, &w, &h)) {
        return;
    }

    px = raster_pack(color);
    for (row = y; row < y + h; ++row) {
        uint32_t *line = m->pixels + row * (m->pitch / 4u) + (uint32_t)x;
        for (col = 0; col < w; ++col) {
            line[col] = px;
        }
    }
}

static int raster_edge(int ax, int ay, int bx, int by, int cx, int cy) {
    return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
}

void raster_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
    const svga_mode_t *m = svga_get_mode();
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    int x;
    int y;
    int area;
    uint32_t px;

    if (!m || !m->pixels || m->bpp != 32) {
        return;
    }

    px = raster_pack(color);
    min_x = x0;
    max_x = x0;
    min_y = y0;
    max_y = y0;

    if (x1 < min_x) {
        min_x = x1;
    }
    if (x2 < min_x) {
        min_x = x2;
    }
    if (x1 > max_x) {
        max_x = x1;
    }
    if (x2 > max_x) {
        max_x = x2;
    }
    if (y1 < min_y) {
        min_y = y1;
    }
    if (y2 < min_y) {
        min_y = y2;
    }
    if (y1 > max_y) {
        max_y = y1;
    }
    if (y2 > max_y) {
        max_y = y2;
    }

    if (min_x < 0) {
        min_x = 0;
    }
    if (min_y < 0) {
        min_y = 0;
    }
    if ((uint32_t)max_x >= m->width) {
        max_x = (int)m->width - 1;
    }
    if ((uint32_t)max_y >= m->height) {
        max_y = (int)m->height - 1;
    }

    area = raster_edge(x0, y0, x1, y1, x2, y2);
    if (area == 0) {
        return;
    }

    for (y = min_y; y <= max_y; ++y) {
        for (x = min_x; x <= max_x; ++x) {
            int w0 = raster_edge(x1, y1, x2, y2, x, y);
            int w1 = raster_edge(x2, y2, x0, y0, x, y);
            int w2 = raster_edge(x0, y0, x1, y1, x, y);

            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0)) {
                m->pixels[y * (m->pitch / 4u) + (uint32_t)x] = px;
            }
        }
    }
}
