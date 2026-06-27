/**
 * CoreOS 95 — HAL ARM (stub para port futuro)
 *
 * Implementar UART/pl011, GIC, e MMU para Raspberry Pi / virt machine.
 */
#include "coreos/types.h"
#include "hal/hal.h"

static char g_arm_log[4096];
static size_t g_arm_log_pos;

static void arm_early_init(void) {
    g_arm_log_pos = 0;
}

static void arm_console_putchar(char c) {
    if (g_arm_log_pos + 1 < sizeof(g_arm_log)) {
        g_arm_log[g_arm_log_pos++] = c;
    }
}

static void arm_console_clear(void) {
    g_arm_log_pos = 0;
}

static void arm_cpu_halt(void) {
    for (;;) {
        __asm__ volatile("wfi");
    }
}

static void arm_cpu_brand(char *buffer, size_t size) {
    if (size > 0) {
        buffer[0] = '\0';
    }
}

static const hal_interface_t g_arm_hal = {
    .arch_name = "ARM (AArch32 stub)",
    .cpu_vendor = "ARM",
    .early_init = arm_early_init,
    .late_init = 0,
    .console_putchar = arm_console_putchar,
    .console_clear = arm_console_clear,
    .console_set_color = 0,
    .cpu_halt = arm_cpu_halt,
    .cpu_pause = 0,
    .interrupts_enable = 0,
    .interrupts_disable = 0,
    .cpu_count = 0,
    .cpu_brand = arm_cpu_brand,
};

const hal_interface_t *hal_arch_get_interface(void) {
    return &g_arm_hal;
}
