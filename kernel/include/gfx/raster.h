#ifndef COREOS_GFX_RASTER_H
#define COREOS_GFX_RASTER_H

#include "coreos/types.h"

void raster_fill_rect(int x, int y, int w, int h, uint32_t color);
void raster_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);

#endif
