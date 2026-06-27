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

int serial_data_available(void) {
    return (inb(0x3F8u + 5) & 0x01u) != 0;
}

char serial_getchar(void) {
    while (!serial_data_available()) {
        __asm__ volatile("pause");
    }
    return (char)inb(0x3F8u);
}

int serial_getchar_nonblock(char *c) {
    if (!serial_data_available()) {
        return 0;
    }
    *c = (char)inb(0x3F8u);
    return 1;
}

void serial_gets(char *buf, uint32_t max_len) {
    uint32_t i = 0;
    char c;

    if (!buf || max_len == 0) return;

    while (i < max_len - 1) {
        c = serial_getchar();
        if (c == '\r' || c == '\n') {
            serial_puts("\r\n");
            break;
        }
        if (c == '\b' || c == 0x7F) {
            if (i > 0) {
                --i;
                serial_puts("\b \b");
            }
            continue;
        }
        buf[i++] = c;
        serial_putchar(c);
    }
    buf[i] = '\0';
}
