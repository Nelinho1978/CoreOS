#include "ntos/io.h"
#include "gui/fb.h"
#include "arch/svga.h"

static DRIVER_OBJECT *g_video_driver;
static DEVICE_OBJECT *g_video_device;

NTSTATUS DrvVideoDispatch(IRP *irp) {
    if (!irp) {
        return STATUS_INVALID_PARAMETER;
    }

    switch (irp->IoControlCode) {
    case IOCTL_VIDEO_SET_MODE:
        if (!svga_init_from_boot_info() || !fb_init()) {
            return STATUS_NOT_IMPLEMENTED;
        }
        return STATUS_SUCCESS;
    default:
        return STATUS_NOT_IMPLEMENTED;
    }
}

void DrvVideoRegister(void) {
    if (!NT_SUCCESS(IoCreateDriver("Video", DrvVideoDispatch, &g_video_driver))) {
        return;
    }
    (void)IoCreateDevice(g_video_driver, "\\Device\\Video0", &g_video_device);
}
