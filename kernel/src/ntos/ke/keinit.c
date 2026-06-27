#include "ntos/ke.h"
#include "coreos/printk.h"

extern void KeArchInitSystem(void);
extern void KeArchPhase1Init(void);

void KeInitSystem(void) {
    KeArchInitSystem();
    kputs("[Ke] Kernel Executive (fase 0)\n");
}

void KePhase1Init(void) {
    KeArchPhase1Init();
    kputs("[Ke] Kernel Executive (fase 1 — GDT/IDT/syscalls)\n");
}

void KeIrqDispatch(uint32_t vector) {
    (void)vector;
}
