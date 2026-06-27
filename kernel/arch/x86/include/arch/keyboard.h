#ifndef COREOS_ARCH_X86_KEYBOARD_H
#define COREOS_ARCH_X86_KEYBOARD_H

typedef enum {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_ENTER,
    KEY_ESC,
    KEY_1,
    KEY_2,
} key_event_t;

void x86_keyboard_init(void);
key_event_t x86_keyboard_poll(void);

#endif
