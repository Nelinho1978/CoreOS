#include "arch/vga.h"
#include "arch/ports.h"

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)

static uint8_t g_color = 0x0F;
static size_t g_row;
static size_t g_col;

static uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

static void vga_scroll(void) {
    for (size_t y = 1; y < VGA_ROWS; ++y) {
        for (size_t x = 0; x < VGA_COLS; ++x) {
            VGA_MEMORY[(y - 1) * VGA_COLS + x] = VGA_MEMORY[y * VGA_COLS + x];
        }
    }

    for (size_t x = 0; x < VGA_COLS; ++x) {
        VGA_MEMORY[(VGA_ROWS - 1) * VGA_COLS + x] = vga_entry(' ', g_color);
    }
}

static void vga_hide_hw_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
    outb(0x3D4, 0x0B);
    outb(0x3D5, 0x00);
}

void x86_vga_init(void) {
    g_row = 0;
    g_col = 0;
    g_color = 0x1F;
    vga_hide_hw_cursor();
    x86_vga_fill_screen(0x0F, 0x01);
    g_row = 0;
    g_col = 0;
}

void x86_vga_clear(void) {
    for (size_t y = 0; y < VGA_ROWS; ++y) {
        for (size_t x = 0; x < VGA_COLS; ++x) {
            VGA_MEMORY[y * VGA_COLS + x] = vga_entry(' ', g_color);
        }
    }
    g_row = 0;
    g_col = 0;
}

void x86_vga_set_color(uint8_t fg, uint8_t bg) {
    g_color = (uint8_t)(fg | (bg << 4));
}

void x86_vga_fill_screen(uint8_t fg, uint8_t bg) {
    uint8_t color = (uint8_t)(fg | (bg << 4));
    for (size_t y = 0; y < VGA_ROWS; ++y) {
        for (size_t x = 0; x < VGA_COLS; ++x) {
            VGA_MEMORY[y * VGA_COLS + x] = vga_entry(' ', color);
        }
    }
}

void x86_vga_putchar_at(size_t row, size_t col, char c, uint8_t fg, uint8_t bg) {
    if (row >= VGA_ROWS || col >= VGA_COLS) {
        return;
    }
    uint8_t color = (uint8_t)(fg | (bg << 4));
    VGA_MEMORY[row * VGA_COLS + col] = vga_entry(c, color);
}

void x86_vga_write_at(size_t row, size_t col, const char *text, uint8_t fg, uint8_t bg) {
    if (!text) {
        return;
    }
    for (size_t i = 0; text[i] != '\0' && col + i < VGA_COLS; ++i) {
        x86_vga_putchar_at(row, col + i, text[i], fg, bg);
    }
}

void x86_vga_fill_span(size_t row, size_t col, size_t len, char ch, uint8_t fg, uint8_t bg) {
    for (size_t i = 0; i < len && col + i < VGA_COLS; ++i) {
        x86_vga_putchar_at(row, col + i, ch, fg, bg);
    }
}

void x86_vga_set_cursor(size_t row, size_t col) {
    g_row = row < VGA_ROWS ? row : VGA_ROWS - 1;
    g_col = col < VGA_COLS ? col : VGA_COLS - 1;
}

void x86_vga_putchar(char c) {
    if (c == '\n') {
        g_col = 0;
        if (++g_row >= VGA_ROWS) {
            vga_scroll();
            g_row = VGA_ROWS - 1;
        }
        return;
    }

    if (c == '\r') {
        g_col = 0;
        return;
    }

    VGA_MEMORY[g_row * VGA_COLS + g_col] = vga_entry(c, g_color);
    if (++g_col >= VGA_COLS) {
        g_col = 0;
        if (++g_row >= VGA_ROWS) {
            vga_scroll();
            g_row = VGA_ROWS - 1;
        }
    }
}
