#include "arch/ports.h"

void serial_init(void) {
    outb(0x3F8u + 1, 0x00);  /* Disable all interrupts */
    outb(0x3F8u + 3, 0x80);  /* Enable DLAB (set baud rate divisor) */
    outb(0x3F8u + 0, 0x0C);  /* Set divisor to 12 (9600 baud) */
    outb(0x3F8u + 1, 0x00);  /* hi byte */
    outb(0x3F8u + 3, 0x03);  /* 8 bits, no parity, 1 stop bit */
    outb(0x3F8u + 2, 0xC7);  /* Enable FIFO, clear, 14-byte threshold */
    outb(0x3F8u + 4, 0x0B);  /* IRQs enabled, RTS/DSR set */
}

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
