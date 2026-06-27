#ifndef COREOS_ARCH_X86_VGA_H
#define COREOS_ARCH_X86_VGA_H

#include "coreos/types.h"

#define VGA_COLS 80u
#define VGA_ROWS 25u

void x86_vga_init(void);
void x86_vga_clear(void);
void x86_vga_putchar(char c);
void x86_vga_set_color(uint8_t fg, uint8_t bg);
void x86_vga_fill_screen(uint8_t fg, uint8_t bg);
void x86_vga_putchar_at(size_t row, size_t col, char c, uint8_t fg, uint8_t bg);
void x86_vga_write_at(size_t row, size_t col, const char *text, uint8_t fg, uint8_t bg);
void x86_vga_fill_span(size_t row, size_t col, size_t len, char ch, uint8_t fg, uint8_t bg);
void x86_vga_set_cursor(size_t row, size_t col);

#endif
