#include "ntos/io.h"
#include "arch/mouse.h"
#include "gui/fb.h"
#include "coreos/printk.h"

static DRIVER_OBJECT *g_mouse_driver;
static DEVICE_OBJECT *g_mouse_device;

NTSTATUS DrvMouseDispatch(IRP *irp) {
    const framebuffer_t *fb;

    if (!irp) {
        return STATUS_INVALID_PARAMETER;
    }

    switch (irp->IoControlCode) {
    case IOCTL_MOUSE_POLL:
        fb = fb_get();
        if (!fb) {
            return STATUS_NOT_IMPLEMENTED;
        }
        (void)mouse_init(fb->width, fb->height);
        return STATUS_SUCCESS;
    default:
        return STATUS_NOT_IMPLEMENTED;
    }
}

void DrvMouseRegister(void) {
    if (!NT_SUCCESS(IoCreateDriver("Mouse", DrvMouseDispatch, &g_mouse_driver))) {
        return;
    }
    if (!NT_SUCCESS(IoCreateDevice(g_mouse_driver, "\\Device\\PointerClass0", &g_mouse_device))) {
        return;
    }
}
