#include "gfx/opengl.h"
#include "gfx/raster.h"
#include "gui/fb.h"

static struct {
    int viewport_x;
    int viewport_y;
    int viewport_w;
    int viewport_h;
    uint8_t clear_r;
    uint8_t clear_g;
    uint8_t clear_b;
    uint32_t mode;
    uint8_t color_r;
    uint8_t color_g;
    uint8_t color_b;
    int in_begin;
    int vertex_count;
    int verts_x[3];
    int verts_y[3];
    int ready;
} g_gl;

static uint32_t gl_pack_color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

int gl_init(void) {
    const framebuffer_t *fb = fb_get();

    g_gl.viewport_x = 0;
    g_gl.viewport_y = 0;
    g_gl.viewport_w = fb ? (int)fb->width : 800;
    g_gl.viewport_h = fb ? (int)fb->height : 600;
    g_gl.clear_r = 0;
    g_gl.clear_g = 0;
    g_gl.clear_b = 0;
    g_gl.color_r = 255;
    g_gl.color_g = 255;
    g_gl.color_b = 255;
    g_gl.in_begin = 0;
    g_gl.vertex_count = 0;
    g_gl.ready = 1;
    return 1;
}

void glViewport(int x, int y, int w, int h) {
    g_gl.viewport_x = x;
    g_gl.viewport_y = y;
    g_gl.viewport_w = w > 0 ? w : 1;
    g_gl.viewport_h = h > 0 ? h : 1;
}

void glClearColor(float r, float g, float b, float a) {
    (void)a;
    if (r < 0.0f) {
        r = 0.0f;
    }
    if (g < 0.0f) {
        g = 0.0f;
    }
    if (b < 0.0f) {
        b = 0.0f;
    }
    if (r > 1.0f) {
        r = 1.0f;
    }
    if (g > 1.0f) {
        g = 1.0f;
    }
    if (b > 1.0f) {
        b = 1.0f;
    }
    g_gl.clear_r = (uint8_t)(r * 255.0f);
    g_gl.clear_g = (uint8_t)(g * 255.0f);
    g_gl.clear_b = (uint8_t)(b * 255.0f);
}

void glClear(uint32_t mask) {
    if ((mask & GL_COLOR_BUFFER_BIT) == 0u || !g_gl.ready) {
        return;
    }
    raster_fill_rect(g_gl.viewport_x, g_gl.viewport_y, g_gl.viewport_w, g_gl.viewport_h,
                     gl_pack_color(g_gl.clear_r, g_gl.clear_g, g_gl.clear_b));
}

void glColor4ub(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    (void)a;
    g_gl.color_r = r;
    g_gl.color_g = g;
    g_gl.color_b = b;
}

void glBegin(uint32_t mode) {
    g_gl.mode = mode;
    g_gl.in_begin = 1;
    g_gl.vertex_count = 0;
}

void glVertex2i(int x, int y) {
    if (!g_gl.in_begin || g_gl.vertex_count >= 3) {
        return;
    }
    g_gl.verts_x[g_gl.vertex_count] = x;
    g_gl.verts_y[g_gl.vertex_count] = y;
    ++g_gl.vertex_count;
}

void glEnd(void) {
    uint32_t color;

    if (!g_gl.in_begin) {
        return;
    }

    g_gl.in_begin = 0;

    if (g_gl.mode != GL_TRIANGLES || g_gl.vertex_count != 3) {
        g_gl.vertex_count = 0;
        return;
    }

    color = gl_pack_color(g_gl.color_r, g_gl.color_g, g_gl.color_b);
    raster_triangle(g_gl.verts_x[0], g_gl.verts_y[0],
                    g_gl.verts_x[1], g_gl.verts_y[1],
                    g_gl.verts_x[2], g_gl.verts_y[2],
                    color);
    g_gl.vertex_count = 0;
}

void glFlush(void) {
}

int gl_esta_pronto(void) {
    return g_gl.ready;
}
