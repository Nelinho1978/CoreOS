#ifndef COREOS_GFX_ACCEL_H
#define COREOS_GFX_ACCEL_H

#include "coreos/types.h"

typedef enum {
    GFX_BACKEND_NONE = 0,
    GFX_BACKEND_CPU_FAST,
    GFX_BACKEND_BOCHS_SVGA,
} gfx_backend_t;

typedef struct gfx_gpu_info {
    char name[48];
    uint16_t vendor_id;
    uint16_t device_id;
    int present;
} gfx_gpu_info_t;

typedef struct gfx_accel_state {
    gfx_backend_t backend;
    int opengl_ok;
    int directx_ok;
    int sse_ok;
    gfx_gpu_info_t gpu;
} gfx_accel_state_t;

int gfx_accel_init(void);
const gfx_accel_state_t *gfx_accel_estado(void);
const char *gfx_accel_backend_nome(void);
int gfx_accel_fill_rect(int x, int y, int w, int h, uint32_t color);
int gfx_accel_blit_rect(const uint32_t *src, int sw, int dx, int dy, int dw, int dh);

#endif
