#ifndef COREOS_ARCH_X86_MOUSE_H
#define COREOS_ARCH_X86_MOUSE_H

#include "coreos/types.h"

int mouse_init(uint32_t screen_w, uint32_t screen_h);
void mouse_poll(void);
int32_t mouse_x(void);
int32_t mouse_y(void);
uint8_t mouse_buttons(void);
int mouse_left_pressed(void);
void mouse_hide_cursor(void);
void mouse_draw_cursor(void);
void mouse_sync_cursor(void);
int mouse_moved(void);

#endif
