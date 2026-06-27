#include "nt/ntstatus.h"
#include "coreos/printk.h"

extern uint32_t ke_syscall_dispatch(uint32_t syscall_number, uint32_t arg0, uint32_t arg1);

uint32_t ke_syscall_dispatch(uint32_t syscall_number, uint32_t arg0, uint32_t arg1) {
    (void)arg0;
    (void)arg1;

    switch (syscall_number) {
    case 0:
        return (uint32_t)STATUS_SUCCESS;
    case 1:
        kputs("[Nt] NtQuerySystemInformation (stub)\n");
        return (uint32_t)STATUS_SUCCESS;
    default:
        return (uint32_t)STATUS_NOT_IMPLEMENTED;
    }
}

void ke_irq_handler_c(void) {
    /* IRQ generica — drivers em modo polling por enquanto */
}
