#include "arch/ke.h"
#include "coreos/printk.h"

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdtr_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idtr_t;

typedef struct {
    uint16_t offset_lo;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_hi;
} __attribute__((packed)) idt_entry_t;

static uint64_t g_gdt[6];
static idt_entry_t g_idt[256];

extern void ke_irq_stub(void);
extern void ke_syscall_stub(void);

static void gdt_set_entry(int index, uint32_t base, uint32_t limit, uint8_t access) {
    uint64_t *entry = &g_gdt[index];
    *entry = 0;
    *entry |= ((uint64_t)limit & 0xFFFFu);
    *entry |= ((uint64_t)base & 0xFFFFFFu) << 16;
    *entry |= ((uint64_t)(limit >> 16) & 0xFu) << 48;
    *entry |= ((uint64_t)(base >> 24) & 0xFFu) << 56;
    *entry |= ((uint64_t)access) << 40;
    *entry |= 0xC0000000000000ull;
}

void KeLoadGdt(void) {
    gdtr_t gdtr;

    gdt_set_entry(0, 0, 0, 0);
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A); /* kernel code ring 0 */
    gdt_set_entry(2, 0, 0xFFFFF, 0x92); /* kernel data ring 0 */
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA); /* user code ring 3 */
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2); /* user data ring 3 */

    gdtr.limit = (uint16_t)(sizeof(g_gdt) - 1u);
    gdtr.base = (uint32_t)g_gdt;
    __asm__ volatile("lgdt %0" ::"m"(gdtr) : "memory");
    __asm__ volatile(
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "ljmp $0x08, $1f\n"
        "1:\n"
        ::: "eax", "memory");
}

static void idt_set_gate(int vector, uint32_t handler, uint16_t selector, uint8_t type_attr) {
    g_idt[vector].offset_lo = (uint16_t)(handler & 0xFFFFu);
    g_idt[vector].selector = selector;
    g_idt[vector].zero = 0;
    g_idt[vector].type_attr = type_attr;
    g_idt[vector].offset_hi = (uint16_t)((handler >> 16) & 0xFFFFu);
}

void KeLoadIdt(void) {
    idtr_t idtr;
    int i;

    for (i = 0; i < 32; ++i) {
        idt_set_gate(i, (uint32_t)ke_irq_stub, 0x08, 0x8E);
    }
    idt_set_gate(0x2E, (uint32_t)ke_syscall_stub, 0x08, 0xEE);

    idtr.limit = (uint16_t)(sizeof(g_idt) - 1u);
    idtr.base = (uint32_t)g_idt;
    __asm__ volatile("lidt %0" ::"m"(idtr) : "memory");
}

void KeEnableSyscalls(void) {
    /* INT 0x2E reservado para chamadas Nt* (estilo NT antigo) */
}

void KeArchInitSystem(void) {
    kputs("[Ke/x86] arquitetura i386 detectada\n");
}

void KeArchPhase1Init(void) {
    /* GDT/IDT opcional — adiado para nao quebrar VGA texto no boot */
    kputs("[Ke/x86] fase 1 (GDT/IDT adiados ate user-mode)\n");
}
