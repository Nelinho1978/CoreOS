#ifndef COREOS_GUI_FB_H
#define COREOS_GUI_FB_H

#include "coreos/types.h"

typedef struct framebuffer {
    uint32_t *pixels;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
} framebuffer_t;

int fb_init(void);
const framebuffer_t *fb_get(void);
uint8_t fb_bpp(void);
uint32_t fb_get_pixel(int x, int y);
void fb_put_pixel(int x, int y, uint32_t color);
void fb_fill_rect(int x, int y, int w, int h, uint32_t color);
void fb_hline(int x, int y, int w, uint32_t color);
void fb_vline(int x, int y, int h, uint32_t color);
void fb_draw_rect(int x, int y, int w, int h, uint32_t color);
void fb_blit_char(int x, int y, char c, uint32_t fg, uint32_t bg);
void fb_draw_string(int x, int y, const char *text, uint32_t fg, uint32_t bg);
void fb_draw_string_transparent(int x, int y, const char *text, uint32_t fg);

#endif
