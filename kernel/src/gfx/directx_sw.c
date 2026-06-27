#include "gfx/directx.h"
#include "gfx/raster.h"
#include "gui/fb.h"

struct d3d_device {
    int viewport_x;
    int viewport_y;
    int viewport_w;
    int viewport_h;
    int ready;
};

static d3d_device_t g_default_device;
static int g_d3d_ready;

int d3d_init(void) {
    const framebuffer_t *fb = fb_get();

    g_default_device.viewport_x = 0;
    g_default_device.viewport_y = 0;
    g_default_device.viewport_w = fb ? (int)fb->width : 800;
    g_default_device.viewport_h = fb ? (int)fb->height : 600;
    g_default_device.ready = 1;
    g_d3d_ready = 1;
    return 1;
}

d3d_device_t *d3d_create_device(void) {
    if (!g_d3d_ready) {
        return NULL;
    }
    return &g_default_device;
}

int d3d_device_clear(d3d_device_t *dev, uint32_t flags, uint32_t color) {
    if (!dev || !dev->ready || (flags & D3DCLEAR_TARGET) == 0u) {
        return 0;
    }
    raster_fill_rect(dev->viewport_x, dev->viewport_y, dev->viewport_w, dev->viewport_h, color);
    return 1;
}

int d3d_device_draw_primitive(d3d_device_t *dev, uint32_t type,
                              const d3d_vertex_t *verts, uint32_t count) {
    uint32_t i;

    if (!dev || !dev->ready || !verts || type != D3DPT_TRIANGLELIST) {
        return 0;
    }

    for (i = 0; i + 2 < count; i += 3) {
        raster_triangle(verts[i].x, verts[i].y,
                        verts[i + 1].x, verts[i + 1].y,
                        verts[i + 2].x, verts[i + 2].y,
                        verts[i].color);
    }
    return 1;
}

int d3d_device_present(d3d_device_t *dev) {
    (void)dev;
    return g_d3d_ready;
}

int d3d_esta_pronto(void) {
    return g_d3d_ready;
}
