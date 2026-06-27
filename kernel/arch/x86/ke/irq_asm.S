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
    ; Syscall via INT 0x2E
    ; Parametros passados em: eax=numero, ebx=arg0, ecx=arg1, edx=arg2
    ; (convencao padrao usada pelo user-mode para INT 0x2E)
    push ebp
    mov ebp, esp
    push edx          ; arg2
    push ecx          ; arg1
    push ebx          ; arg0
    push eax          ; syscall_number
    call ke_syscall_dispatch
    add esp, 16
    pop ebp
    iretd
