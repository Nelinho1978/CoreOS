#include "arch/irq.h"
#include "arch/ports.h"

void x86_irq_disable_all(void) {
    __asm__ volatile("cli");

    /* EOI pendente no PIC */
    (void)inb(0x60);
    (void)inb(0x20);
    (void)inb(0xA0);

    outb(0x21, 0xFFu);
    outb(0xA1, 0xFFu);
}
