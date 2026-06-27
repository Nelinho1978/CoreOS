#include "hal/hal.h"

static const hal_interface_t *g_hal;

void hal_register(const hal_interface_t *iface) {
    g_hal = iface;
}

const hal_interface_t *hal_get(void) {
    return g_hal;
}

void hal_early_init(void) {
    if (g_hal && g_hal->early_init) {
        g_hal->early_init();
    }
}

void hal_late_init(void) {
    if (g_hal && g_hal->late_init) {
        g_hal->late_init();
    }
}

void hal_putchar(char c) {
    if (g_hal && g_hal->console_putchar) {
        g_hal->console_putchar(c);
    }
}

void hal_puts(const char *s) {
    while (*s) {
        hal_putchar(*s++);
    }
}

void hal_clear_screen(void) {
    if (g_hal && g_hal->console_clear) {
        g_hal->console_clear();
    }
}

void hal_halt(void) {
    if (g_hal && g_hal->cpu_halt) {
        g_hal->cpu_halt();
    }
    for (;;) {
        __asm__ volatile("hlt");
    }
}

const char *hal_arch_name(void) {
    return g_hal ? g_hal->arch_name : "unknown";
}
