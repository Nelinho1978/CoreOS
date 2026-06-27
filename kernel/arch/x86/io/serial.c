#include "arch/ports.h"

void serial_putchar(char c) {
    uint32_t timeout = 65536u;

    if (c == '\n') {
        serial_putchar('\r');
    }

    while (timeout-- > 0u && (inb(0x3FDu) & 0x20u) == 0u) {
        __asm__ volatile("pause");
    }
    outb(0x3F8u, (uint8_t)c);
}

void serial_puts(const char *s) {
    if (!s) {
        return;
    }
    while (*s) {
        serial_putchar(*s++);
    }
}
