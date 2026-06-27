; CoreOS 10 — ponto de entrada do kernel (32-bit)
; Firmware (stage2) ja executou: Video + CarregarKernel
; Agora: jmp Kernel

section .note.GNU-stack noalloc noexec nowrite progbits

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text.start
global _start
extern kernel_main
extern __bss_start
extern __bss_end
extern serial_init

COREOS_BOOT_MAGIC equ 0xC0950095
BOOT_INFO_ADDR    equ 0x9000

_start:
    cli
    mov al, 0xFF
    out 0x21, al
    out 0xA1, al

    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    jz .bss_done
    cld
    xor eax, eax
    rep stosb
.bss_done:
    mov edi, BOOT_INFO_ADDR
    cmp dword [edi], 0xFB950095
    je .skip_boot_info_clear
    xor eax, eax
    mov ecx, 32
    cld
    rep stosb
.skip_boot_info_clear:
    mov esp, stack_top
    and esp, 0xfffffff0
    call serial_init
    ; Debug: signal kernel is running
    mov dx, 0x3F8
    mov al, 'K'
    out dx, al
    mov al, 'E'
    out dx, al
    push BOOT_INFO_ADDR
    push COREOS_BOOT_MAGIC
    mov eax, kernel_main
    jmp eax

.hang:
    cli
    hlt
    jmp .hang
