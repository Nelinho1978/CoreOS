#include "ntos/io.h"
#include "arch/keyboard.h"
#include "coreos/printk.h"

static DRIVER_OBJECT *g_kbd_driver;
static DEVICE_OBJECT *g_kbd_device;

NTSTATUS DrvKeyboardDispatch(IRP *irp) {
    if (!irp) {
        return STATUS_INVALID_PARAMETER;
    }

    switch (irp->IoControlCode) {
    case IOCTL_KBD_READ:
        x86_keyboard_init();
        return STATUS_SUCCESS;
    default:
        return STATUS_NOT_IMPLEMENTED;
    }
}

void DrvKeyboardRegister(void) {
    if (!NT_SUCCESS(IoCreateDriver("Keyboard", DrvKeyboardDispatch, &g_kbd_driver))) {
        return;
    }
    (void)IoCreateDevice(g_kbd_driver, "\\Device\\KeyboardClass0", &g_kbd_device);
}
