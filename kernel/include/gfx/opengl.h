#ifndef COREOS_GFX_OPENGL_H
#define COREOS_GFX_OPENGL_H

#include "coreos/types.h"

#define GL_COLOR_BUFFER_BIT 0x00004000u
#define GL_TRIANGLES        0x00000004u

int gl_init(void);
void glViewport(int x, int y, int w, int h);
void glClearColor(float r, float g, float b, float a);
void glClear(uint32_t mask);
void glColor4ub(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void glBegin(uint32_t mode);
void glVertex2i(int x, int y);
void glEnd(void);
void glFlush(void);
int gl_esta_pronto(void);

#endif
