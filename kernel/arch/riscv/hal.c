/**
 * CoreOS 95 — HAL RISC-V (stub para port futuro)
 *
 * Implementar SBI console, trap handler e paginacao Sv39.
 */
#include "coreos/types.h"
#include "hal/hal.h"

static char g_riscv_log[4096];
static size_t g_riscv_log_pos;

static void riscv_early_init(void) {
    g_riscv_log_pos = 0;
}

static void riscv_console_putchar(char c) {
    if (g_riscv_log_pos + 1 < sizeof(g_riscv_log)) {
        g_riscv_log[g_riscv_log_pos++] = c;
    }
}

static void riscv_console_clear(void) {
    g_riscv_log_pos = 0;
}

static void riscv_cpu_halt(void) {
    for (;;) {
        __asm__ volatile("wfi");
    }
}

static void riscv_cpu_brand(char *buffer, size_t size) {
    if (size > 0) {
        buffer[0] = '\0';
    }
}

static const hal_interface_t g_riscv_hal = {
    .arch_name = "RISC-V (stub)",
    .cpu_vendor = "RISC-V",
    .early_init = riscv_early_init,
    .late_init = 0,
    .console_putchar = riscv_console_putchar,
    .console_clear = riscv_console_clear,
    .console_set_color = 0,
    .cpu_halt = riscv_cpu_halt,
    .cpu_pause = 0,
    .interrupts_enable = 0,
    .interrupts_disable = 0,
    .cpu_count = 0,
    .cpu_brand = riscv_cpu_brand,
};

const hal_interface_t *hal_arch_get_interface(void) {
    return &g_riscv_hal;
}
