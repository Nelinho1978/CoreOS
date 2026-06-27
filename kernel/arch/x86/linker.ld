/* CoreOS 95 — kernel carregado pelo boot sector em 0x10000 */
ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text : ALIGN(4) {
        *(.text.start)
        *(.text*)
    }

    .rodata : ALIGN(4) {
        *(.rodata*)
    }

    .data : ALIGN(4) {
        *(.data*)
    }

    __bss_start = .;
    .bss : ALIGN(4) {
        *(COMMON)
        *(.bss*)
    }
    __bss_end = .;
}
