; CoreOS 10 — MBR: carrega stage2 e transfere controle

[BITS 16]

STAGE2_LOAD_SEG  equ 0x07E0
STAGE2_SECTORS   equ 8
STAGE2_START_LBA equ 1
DISK_SECTORS     equ 2880

org 0x7C00
    jmp short start
    times 8 - ($ - $$) db 0

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    mov [boot_drive], dl
    in al, 0x92
    or al, 2
    out 0x92, al
    mov ax, 0x0003
    int 0x10
    mov si, msg_load
    call print16
    call load_stage2
    jc disk_error
    mov si, msg_ok
    call print16
    jmp STAGE2_LOAD_SEG:0

print16:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    int 0x10
    jmp print16
.done:
    ret

load_stage2:
    mov word [cur_lba], STAGE2_START_LBA
    mov cx, STAGE2_SECTORS
    mov ax, STAGE2_LOAD_SEG
    mov es, ax
    xor bx, bx
.loop:
    push cx
    mov ax, [cur_lba]
    xor dx, dx
    mov cx, 18
    div cx
    mov cl, dl
    inc cl
    mov [sect], cl
    push ax
    and ax, 1
    mov [head], al
    pop ax
    shr ax, 1
    mov ch, al
    mov ah, 0x02
    mov al, 1
    mov dh, [head]
    mov cl, [sect]
    mov dl, [boot_drive]
    int 0x13
    pop cx
    jc .fail
    add bx, 0x200
    inc word [cur_lba]
    loop .loop
    clc
    ret
.fail:
    pop cx
    stc
    ret

disk_error:
    mov si, msg_err
    call print16
.halt:
    hlt
    jmp .halt

boot_drive db 0
cur_lba    dw 0
head       db 0
sect       db 0
msg_load db "CoreOS 10 boot...", 13, 10, 0
msg_ok   db "Stage2 OK", 13, 10, 0
msg_err  db "Boot ERRO", 13, 10, 0

times 0x1BE - ($ - $$) db 0
db 0x80, 0, 1, 1, 0x01, 0xFE, 0x3F, 0x6F
dd 0, DISK_SECTORS
times 0x1FE - ($ - $$) db 0
dw 0xAA55
