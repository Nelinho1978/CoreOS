; CoreOS 10 — Stage2 (firmware real-mode)
;
;   InicializarVideo  -> VBE/SVGA, boot_info em 0x8000
;   CarregarKernel    -> kernel.bin para 0x10000
;   jmp Kernel        -> modo protegido

[BITS 16]
org 0x7E00

KERNEL_LOAD_SEG   equ 0x1000
KERNEL_SECTORS    equ 200
KERNEL_START_LBA  equ 9
STAGE2_SECTORS    equ 8
CODE_SEG          equ 8
DATA_SEG          equ 16

VBE_MODE_LINEAR   equ 0x4000
BOOT_FB_MAGIC     equ 0xFB950095
DISPLAY_INFO_ADDR equ 0x9020
DISPLAY_INFO_MAGIC equ 0xD15010A1
DISPLAY_MAX_MODES equ 12

VBE_INFO_BUF      equ 0x5000
MODE_INFO_BUF     equ 0x6000
BOOT_INFO_ADDR    equ 0x9000

; serial_putc: write AL to COM1 (no LSR check, for QEMU speed)
serial_putc:
    push dx
    mov dx, 0x3F8
    out dx, al
    pop dx
    ret

; debug_print: print null-terminated string at CS:SI to COM1
debug_print:
    push ax
    push si
    push dx
    mov dx, 0x3F8
.loop:
    lodsb
    test al, al
    jz .done
    out dx, al
    jmp .loop
.done:
    pop dx
    pop si
    pop ax
    ret

stage2:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000
    mov [boot_drive], dl

    ; RAW OUT test (no function calls - like 'U!' test)
    mov dx, 0x3F8
    mov al, '1'
    out dx, al

    mov ah, 0x00
    mov dl, [boot_drive]
    int 0x13

    mov dx, 0x3F8
    mov al, '2'
    out dx, al

    call enable_a20

    mov dx, 0x3F8
    mov al, '3'
    out dx, al

    ; BYPASS VESA — go straight to VGA mode 0x13 + kernel load
    jmp vga13_fallback

.video_ok:
    call vesa_finalize_display
    jmp load_kernel_and_boot

vga13_fallback:
    mov dx, 0x3F8
    mov al, 'V'
    out dx, al
    mov ax, 0x0013
    int 0x10
    mov ax, BOOT_INFO_ADDR >> 4
    mov es, ax
    xor di, di
    mov dword [es:di], BOOT_FB_MAGIC
    mov dword [es:di + 4], 320
    mov dword [es:di + 8], 200
    mov dword [es:di + 12], 320
    mov dword [es:di + 16], 8
    mov dword [es:di + 20], 0xA0000
    mov word [selected_vbe_mode], 0x013
    call vesa_fallback_display

load_kernel_and_boot:
    mov dx, 0x3F8
    mov al, 'K'
    out dx, al
    call load_kernel_image
    mov dx, 0x3F8
    mov al, 'L'
    out dx, al
    jnc enter_protected
    mov dx, 0x3F8
    mov al, 'F'
    out dx, al
    jmp load_error

enter_protected:
    mov al, 'P'       ; marker: entering protected mode
    call serial_putc
    cli
    ; Re-write GDT in case VBE BIOS corrupted it
    xor eax, eax
    mov [gdt + 0], eax
    mov [gdt + 4], eax
    mov dword [gdt + 8], 0x0000FFFF
    mov dword [gdt + 12], 0x00CF9A00
    mov dword [gdt + 16], 0x0000FFFF
    mov dword [gdt + 20], 0x00CF9200
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:enter_pm

enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

vesa_scan_modes:
    pusha
    mov ax, 0x4F00
    mov di, VBE_INFO_BUF
    int 0x10
    cmp ax, 0x004F
    jne .scan_fail
    cmp dword [VBE_INFO_BUF], 0x32454256
    jne .scan_fail

    mov ax, DISPLAY_INFO_ADDR >> 4
    mov es, ax
    xor di, di
    mov dword [es:di], DISPLAY_INFO_MAGIC
    mov ax, [VBE_INFO_BUF + 4]
    mov [es:di + 4], ax
    mov word [es:di + 8], 0
    mov word [es:di + 10], 0
    mov word [es:di + 12], 800
    mov word [es:di + 14], 600
    mov byte [es:di + 16], 0
    mov byte [es:di + 17], 1

    push ds
    mov ax, [VBE_INFO_BUF + 16]
    mov ds, ax
    mov si, [VBE_INFO_BUF + 14]
    xor bx, bx
.mode_loop:
    mov cx, [si]
    cmp cx, 0xFFFF
    je .scan_done
    cmp bl, DISPLAY_MAX_MODES
    jae .scan_done

    push bx
    push si
    mov ax, 0x4F01
    mov di, MODE_INFO_BUF
    int 0x10
    pop si
    pop bx
    cmp ax, 0x004F
    jne .next_mode
    test byte [MODE_INFO_BUF], 0x80
    jz .next_mode

    push bx
    push si
    mov ax, DISPLAY_INFO_ADDR >> 4
    mov es, ax
    xor di, di
    movzx eax, word [MODE_INFO_BUF + 18]
    cmp ax, [es:di + 8]
    jbe .max_h_ok
    mov [es:di + 8], ax
.max_h_ok:
    movzx eax, word [MODE_INFO_BUF + 20]
    cmp ax, [es:di + 10]
    jbe .max_v_ok
    mov [es:di + 10], ax
.max_v_ok:
    mov di, bx
    shl di, 3
    add di, 20
    mov ax, [si]
    mov [es:di], ax
    movzx eax, word [MODE_INFO_BUF + 18]
    mov [es:di + 2], ax
    movzx eax, word [MODE_INFO_BUF + 20]
    mov [es:di + 4], ax
    movzx eax, byte [MODE_INFO_BUF + 22]
    mov [es:di + 6], al
    mov byte [es:di + 7], 1
    inc bl
    mov ax, DISPLAY_INFO_ADDR >> 4
    mov es, ax
    xor di, di
    mov [es:di + 16], bl
    pop si
    pop bx
.next_mode:
    add si, 2
    jmp .mode_loop
.scan_done:
    pop ds
    popa
    ret
.scan_fail:
    mov ax, DISPLAY_INFO_ADDR >> 4
    mov es, ax
    xor di, di
    mov dword [es:di], DISPLAY_INFO_MAGIC
    mov word [es:di + 12], 800
    mov word [es:di + 14], 600
    mov byte [es:di + 17], 0
    popa
    ret

vesa_finalize_display:
    push ax
    push es
    push di
    mov ax, DISPLAY_INFO_ADDR >> 4
    mov es, ax
    xor di, di
    mov ax, [selected_vbe_mode]
    mov [es:di + 6], ax
    pop di
    pop es
    pop ax
    ret

vesa_fallback_display:
    push ax
    push es
    push di
    mov ax, DISPLAY_INFO_ADDR >> 4
    mov es, ax
    xor di, di
    mov dword [es:di], DISPLAY_INFO_MAGIC
    mov word [es:di + 8], 320
    mov word [es:di + 10], 200
    mov word [es:di + 12], 800
    mov word [es:di + 14], 600
    mov byte [es:di + 16], 1
    mov byte [es:di + 17], 1
    mov word [es:di + 6], 0x013
    mov di, 20
    mov word [es:di], 0x013
    mov word [es:di + 2], 320
    mov word [es:di + 4], 200
    mov byte [es:di + 6], 8
    mov byte [es:di + 7], 0
    pop di
    pop es
    pop ax
    ret

vesa_try_one:
    push cx
    mov ax, 0x4F00
    mov di, VBE_INFO_BUF
    int 0x10
    cmp ax, 0x004F
    jne .fail
    cmp dword [VBE_INFO_BUF], 0x32454256
    jne .fail

    pop cx
    push cx
    mov ax, 0x4F02
    mov bx, cx
    or bx, VBE_MODE_LINEAR
    int 0x10
    cmp ax, 0x004F
    jne .fail

    mov ax, 0x4F01
    pop cx
    push cx
    mov di, MODE_INFO_BUF
    int 0x10
    cmp ax, 0x004F
    jne .fail
    cmp dword [MODE_INFO_BUF + 40], 0
    je .fail

    mov ax, BOOT_INFO_ADDR >> 4
    mov es, ax
    xor di, di
    mov dword [es:di], BOOT_FB_MAGIC
    movzx eax, word [MODE_INFO_BUF + 18]
    mov [es:di + 4], eax
    movzx eax, word [MODE_INFO_BUF + 20]
    mov [es:di + 8], eax
    movzx eax, word [MODE_INFO_BUF + 28]
    mov [es:di + 12], eax
    movzx eax, byte [MODE_INFO_BUF + 22]
    mov [es:di + 16], eax
    mov eax, [MODE_INFO_BUF + 40]
    mov [es:di + 20], eax
    mov [selected_vbe_mode], cx
    pop cx
    clc
    ret
.fail:
    pop cx
    stc
    ret

load_kernel_image:
    mov word [cur_lba], KERNEL_START_LBA
    mov cx, KERNEL_SECTORS
    mov ax, KERNEL_LOAD_SEG
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

load_error:
    mov si, msg_err
    call print16
.halt:
    hlt
    jmp .halt

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

[BITS 32]
enter_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    mov eax, 0x10000
    jmp eax

[BITS 16]
boot_drive db 0
cur_lba    dw 0
head       db 0
sect       db 0
selected_vbe_mode dw 0
msg_err db "ERRO kernel", 13, 10, 0

align 8
gdt:
    dq 0
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_descriptor:
    dw gdt_descriptor - gdt - 1
    dd gdt

times (STAGE2_SECTORS * 512) - ($ - $$) db 0
