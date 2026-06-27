; CoreOS 10 — Scheduler context switch (x86 assembly)
; Switches between thread kernel stacks

[BITS 32]
section .text

; --------------------------------------------------------------
; void sched_context_switch(uint32_t *old_esp, uint32_t new_esp)
;
; Saves all registers to the current stack, then switches
; to the new thread's stack and restores all registers.
; --------------------------------------------------------------
global sched_context_switch
sched_context_switch:
    ; Save current context
    push ebp
    push ebx
    push esi
    push edi
    push ds
    push es
    push fs
    push gs

    ; Save current stack pointer
    mov eax, [esp + 36]     ; old_esp = first param
    mov [eax], esp          ; *old_esp = current esp

    ; Load new stack pointer
    mov eax, [esp + 40]     ; new_esp = second param
    mov esp, eax

    ; Restore new context
    pop gs
    pop fs
    pop es
    pop ds
    pop edi
    pop esi
    pop ebx
    pop ebp

    ret

; --------------------------------------------------------------
; void sched_switch_to_user(uint32_t eip, uint32_t esp, uint32_t page_dir)
;
; Switches to user mode (ring 3) by setting up the stack frame
; for an iret that loads user CS, SS, and ESP.
; --------------------------------------------------------------
global sched_switch_to_user
sched_switch_to_user:
    ; eip = [esp+4], esp = [esp+8], page_dir = [esp+12]
    cli

    ; Switch page directory
    mov eax, [esp + 12]     ; page_dir
    mov cr3, eax

    ; Set up kernel stack in TSS (can't access TSS easily here)
    ; Assume TSS.ss0 = 0x10, TSS.esp0 already set

    ; Set up user-mode stack frame:
    ; SS (user data = 0x23 = ring 3 + GDT entry 4)
    ; ESP (user stack)
    ; EFLAGS (IF = 1)
    ; CS (user code = 0x1B = ring 3 + GDT entry 3)
    ; EIP (entry point)

    push dword 0x23          ; SS = user data segment (ring 3)
    push dword [esp + 8]     ; ESP = user stack
    pushfd                   ; EFLAGS
    or dword [esp], 0x200    ; Set IF flag
    push dword 0x1B          ; CS = user code segment (ring 3)
    push dword [esp + 16]    ; EIP = entry point
    iret

; --------------------------------------------------------------
; IRQ0 (PIT timer) handler — calls sched_timer_tick
; --------------------------------------------------------------
extern sched_timer_tick

global irq0_handler
irq0_handler:
    pusha
    push ds
    push es

    mov ax, 0x10            ; Kernel data segment
    mov ds, ax
    mov es, ax

    ; Send EOI to PIC master
    mov al, 0x20
    out 0x20, al

    call sched_timer_tick

    pop es
    pop ds
    popa
    iret
