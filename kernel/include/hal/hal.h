/**
 * HAL — Hardware Abstraction Layer
 *
 * Interface portável: cada arquitetura (x86, ARM, RISC-V) implementa
 * hal_arch_interface e registra no boot. O núcleo do kernel só usa HAL.
 */
#ifndef COREOS_HAL_H
#define COREOS_HAL_H

#include "coreos/types.h"

typedef struct hal_interface {
    const char *arch_name;
    const char *cpu_vendor;

    void (*early_init)(void);
    void (*late_init)(void);

    void (*console_putchar)(char c);
    void (*console_clear)(void);
    void (*console_set_color)(uint8_t fg, uint8_t bg);

    void (*cpu_halt)(void);
    void (*cpu_pause)(void);
    void (*interrupts_enable)(void);
    void (*interrupts_disable)(void);

    uint32_t (*cpu_count)(void);
    void (*cpu_brand)(char *buffer, size_t size);
} hal_interface_t;

void hal_register(const hal_interface_t *iface);
const hal_interface_t *hal_get(void);

void hal_early_init(void);
void hal_late_init(void);
void hal_putchar(char c);
void hal_puts(const char *s);
void hal_clear_screen(void);
void hal_halt(void);
const char *hal_arch_name(void);

#endif
