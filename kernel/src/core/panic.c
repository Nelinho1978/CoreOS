#include "coreos/kernel.h"
#include "coreos/printk.h"
#include "hal/hal.h"

void kernel_panic(const char *message) {
    hal_puts("\n*** KERNEL PANIC ***\n");
    hal_puts(message ? message : "unknown error");
    hal_puts("\nSystem halted.\n");
    hal_halt();
    for (;;) {
    }
}
