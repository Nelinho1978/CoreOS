#include "arch/keyboard.h"
#include "arch/ports.h"

#define KBD_STATUS_PORT 0x64u
#define KBD_DATA_PORT 0x60u

static int kbd_has_data(void) {
    return (inb(KBD_STATUS_PORT) & 0x01u) != 0;
}

static uint8_t kbd_read_scancode(void) {
    while (!kbd_has_data()) {
        __asm__ volatile("pause");
    }
    return inb(KBD_DATA_PORT);
}

static key_event_t scancode_to_event(uint8_t sc) {
    if (sc & 0x80u) {
        return KEY_NONE;
    }

    switch (sc) {
    case 0x1Cu:
        return KEY_ENTER;
    case 0x01u:
        return KEY_ESC;
    case 0x02u:
        return KEY_1;
    case 0x03u:
        return KEY_2;
    default:
        return KEY_NONE;
    }
}

void x86_keyboard_init(void) {
    /* Teclado PS/2 ja funciona na maioria dos emuladores sem init extra. */
}

key_event_t x86_keyboard_poll(void) {
    if (!kbd_has_data()) {
        return KEY_NONE;
    }

    uint8_t sc = kbd_read_scancode();
    if (sc == 0xE0u) {
        if (!kbd_has_data()) {
            return KEY_NONE;
        }
        sc = kbd_read_scancode();
        if (sc & 0x80u) {
            return KEY_NONE;
        }
        if (sc == 0x48u) {
            return KEY_UP;
        }
        if (sc == 0x50u) {
            return KEY_DOWN;
        }
        return KEY_NONE;
    }

    return scancode_to_event(sc);
}
