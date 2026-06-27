#include "arch/ke.h"
#include "coreos/printk.h"
#include "arch/ports.h"
#include "arch/scheduler.h"

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

/* GDT: 0=null, 1=krn_code, 2=krn_data, 3=usr_code, 4=usr_data, 5=TSS */
#define GDT_TSS_IDX 5
static uint64_t g_gdt[6];
static idt_entry_t g_idt[256];

/* TSS defined in sched.c */
extern void tss_init(void);
extern uint32_t tss_get_phys_addr(void);
extern TSS_ENTRY g_tss;

/* Forward declare irq0_handler from sched_asm.S */
extern void irq0_handler(void);

extern void ke_irq_stub(void);
extern void ke_syscall_stub(void);
extern void irq0_handler(void);

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
    uint32_t tss_phys;
    uint64_t tss_desc;

    gdt_set_entry(0, 0, 0, 0);
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A); /* kernel code ring 0 */
    gdt_set_entry(2, 0, 0xFFFFF, 0x92); /* kernel data ring 0 */
    gdt_set_entry(3, 0, 0xFFFFF, 0xFA); /* user code ring 3 */
    gdt_set_entry(4, 0, 0xFFFFF, 0xF2); /* user data ring 3 */

    /* TSS segment descriptor */
    tss_phys = tss_get_phys_addr();
    tss_desc = (uint64_t)(sizeof(TSS_ENTRY) - 1) & 0xFFFF;  /* Limit[15:0] = 103 */
    tss_desc |= ((uint64_t)tss_phys & 0xFFFFFF) << 16;       /* Base[23:0] */
    tss_desc |= (uint64_t)0x89 << 40;                         /* Access: Present, DPL0, 32-bit TSS avail */
    tss_desc |= ((uint64_t)(tss_phys >> 24) & 0xFF) << 56;   /* Base[31:24] */
    g_gdt[GDT_TSS_IDX] = tss_desc;
    kputs("[GDT] TSS descritor configurado\n");

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

    /* Load task register with TSS selector */
    __asm__ volatile("ltr %%ax" ::"a"((uint16_t)(GDT_TSS_IDX * 8)) : "memory");
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
    /* PIT timer IRQ0 -> vector 0x20 */
    idt_set_gate(0x20, (uint32_t)irq0_handler, 0x08, 0x8E);
    /* Syscall via INT 0x2E */
    idt_set_gate(0x2E, (uint32_t)ke_syscall_stub, 0x08, 0xEE);

    idtr.limit = (uint16_t)(sizeof(g_idt) - 1u);
    idtr.base = (uint32_t)g_idt;
    __asm__ volatile("lidt %0" ::"m"(idtr) : "memory");
}

void KeEnableSyscalls(void) {
    /* INT 0x2E reservado para chamadas Nt* (estilo NT antigo) */
}

void ke_unmask_pit_irq(void) {
    /* Unmask IRQ0 on PIC master (clear bit 0 in IMR) */
    uint8_t imr = inb(0x21);
    imr &= ~0x01;
    outb(0x21, imr);
}

void KeArchInitSystem(void) {
    kputs("[Ke/x86] arquitetura i386 detectada\n");
}

/* ---- PIC remapping (ICW2) ---- */
/* Moves IRQ0-7 from INT 0x08-0x0F to INT 0x20-0x27 */
/* Moves IRQ8-15 from INT 0x70-0x77 to INT 0x28-0x2F */
static void pic_remap(void) {
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);

    outb(0x20, 0x11);  /* ICW1: Initialize */
    outb(0xA0, 0x11);
    outb(0x21, 0x20);  /* ICW2: Master PIC vector offset = 0x20 */
    outb(0xA1, 0x28);  /* ICW2: Slave PIC vector offset = 0x28 */
    outb(0x21, 0x04);  /* ICW3: Master has slave at IRQ2 */
    outb(0xA1, 0x02);  /* ICW3: Slave cascade identity */
    outb(0x21, 0x01);  /* ICW4: x86 mode */
    outb(0xA1, 0x01);

    outb(0x21, a1);    /* Restore masks */
    outb(0xA1, a2);

    kputs("[PIC] Remapeado: IRQ0-7->INT 0x20-0x27\n");
}

void KeArchPhase1Init(void) {
    kputs("[Ke/x86] fase 1 — carregando GDT/IDT...\n");
    pic_remap();  /* Remap PIC FIRST so IRQs don't hit INT 0x08-0x0F */
    KeLoadGdt();
    KeLoadIdt();
    ke_unmask_pit_irq();
    kputs("[Ke/x86] GDT, IDT e PIT ativados\n");
}
