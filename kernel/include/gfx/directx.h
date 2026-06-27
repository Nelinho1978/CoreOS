#ifndef COREOS_GFX_DIRECTX_H
#define COREOS_GFX_DIRECTX_H

#include "coreos/types.h"

#define D3DCLEAR_TARGET     0x00000001u
#define D3DPT_TRIANGLELIST  0x00000004u

typedef struct d3d_vertex {
    int x;
    int y;
    uint32_t color;
} d3d_vertex_t;

typedef struct d3d_device d3d_device_t;

int d3d_init(void);
d3d_device_t *d3d_create_device(void);
int d3d_device_clear(d3d_device_t *dev, uint32_t flags, uint32_t color);
int d3d_device_draw_primitive(d3d_device_t *dev, uint32_t type,
                              const d3d_vertex_t *verts, uint32_t count);
int d3d_device_present(d3d_device_t *dev);
int d3d_esta_pronto(void);

#endif
