; Saida serial COM1 (0x3F8) — nao bloqueia para sempre
%ifndef SERIAL_INC
%define SERIAL_INC

%macro SERIAL_PUTC 1
    mov al, %1
    call serial_putc_nb
%endmacro

%macro SERIAL_STR 1
    mov si, %1
%%loop:
    lodsb
    test al, al
    jz %%done
    call serial_putc_nb
    jmp %%loop
%%done:
%endmacro

serial_putc_nb:
    push dx
    push ax
    mov cx, 50000
.spin:
    mov dx, 0x3FD
    in al, dx
    test al, 0x20
    jnz .send
    loop .spin
    jmp .out
.send:
    pop ax
    mov dx, 0x3F8
    out dx, al
    pop dx
    ret
.out:
    pop ax
    pop dx
    ret

%endif
