#include "arch/vga.h"
#include "arch/serial.h"
#include "coreos/boot_info.h"
#include "coreos/types.h"
#include "arch/irq.h"
#include "hal/hal.h"

extern void x86_cpu_brand(char *buffer, size_t size);
extern uint32_t x86_cpu_count(void);

static void x86_early_init(void) {
    x86_irq_disable_all();
    x86_vga_init();
}

static void x86_late_init(void) {
}

static void x86_console_putchar(char c) {
    const boot_framebuffer_t *bi = boot_framebuffer_get();

    serial_putchar(c);
    if (bi->magic != BOOT_FB_MAGIC) {
        x86_vga_putchar(c);
    }
}

static void x86_console_clear(void) {
    x86_vga_clear();
}

static void x86_console_set_color(uint8_t fg, uint8_t bg) {
    x86_vga_set_color(fg, bg);
}

static void x86_cpu_halt(void) {
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

static void x86_cpu_pause(void) {
    __asm__ volatile("pause");
}

static void x86_interrupts_enable(void) {
    __asm__ volatile("sti");
}

static void x86_interrupts_disable(void) {
    __asm__ volatile("cli");
}

static const hal_interface_t g_x86_hal = {
    .arch_name = "x86 (i386)",
    .cpu_vendor = "cpuid",
    .early_init = x86_early_init,
    .late_init = x86_late_init,
    .console_putchar = x86_console_putchar,
    .console_clear = x86_console_clear,
    .console_set_color = x86_console_set_color,
    .cpu_halt = x86_cpu_halt,
    .cpu_pause = x86_cpu_pause,
    .interrupts_enable = x86_interrupts_enable,
    .interrupts_disable = x86_interrupts_disable,
    .cpu_count = x86_cpu_count,
    .cpu_brand = x86_cpu_brand,
};

const hal_interface_t *hal_arch_get_interface(void) {
    return &g_x86_hal;
}
