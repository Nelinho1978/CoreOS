#include <stdarg.h>

#include "coreos/printk.h"
#include "hal/hal.h"

static void print_uint(uint32_t value, uint32_t base) {
    char buffer[12];
    int i = 10;
    buffer[11] = '\0';

    if (value == 0) {
        kputc('0');
        return;
    }

    while (value > 0 && i >= 0) {
        const uint32_t digit = value % base;
        buffer[i--] = (char)(digit < 10 ? '0' + digit : 'a' + digit - 10);
        value /= base;
    }

    kputs(&buffer[i + 1]);
}

void kputc(char c) {
    hal_putchar(c);
}

void kputs(const char *s) {
    if (!s) {
        return;
    }
    hal_puts(s);
}

void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (; *fmt; ++fmt) {
        if (*fmt != '%') {
            kputc(*fmt);
            continue;
        }

        ++fmt;
        switch (*fmt) {
        case 's':
            kputs(va_arg(args, const char *));
            break;
        case 'c':
            kputc((char)va_arg(args, int));
            break;
        case 'd': {
            int32_t value = va_arg(args, int32_t);
            if (value < 0) {
                kputc('-');
                value = -value;
            }
            print_uint((uint32_t)value, 10);
            break;
        }
        case 'u':
            print_uint(va_arg(args, uint32_t), 10);
            break;
        case 'x':
            print_uint(va_arg(args, uint32_t), 16);
            break;
        case '%':
            kputc('%');
            break;
        default:
            kputc('%');
            kputc(*fmt);
            break;
        }
    }

    va_end(args);
}

void kprint_hex(uint32_t value) {
    kprintf("0x%x", value);
}
