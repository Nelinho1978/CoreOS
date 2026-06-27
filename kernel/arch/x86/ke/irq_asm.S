BITS 32

section .text

extern ke_irq_handler_c
extern ke_syscall_dispatch

global ke_irq_stub
global ke_syscall_stub

ke_irq_stub:
    pusha
    call ke_irq_handler_c
    popa
    iretd

ke_syscall_stub:
    push ebp
    mov ebp, esp
    push dword [ebp + 12]
    push dword [ebp + 8]
    push dword [ebp + 4]
    call ke_syscall_dispatch
    add esp, 12
    pop ebp
    iretd
