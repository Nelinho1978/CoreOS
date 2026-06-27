#include "nt/ntstatus.h"
#include "subsys/win32.h"
#include "ntos/cm.h"
#include "ntos/ps.h"
#include "ntos/se.h"
#include "ntos/io.h"
#include "gui/win10.h"
#include "coreos/printk.h"

NTSTATUS Win32SubsystemInitialize(void) {
    char product[CM_VALUE_LEN];

    kputs("[Win32] csrss/win32k: subsistema Win32 inicializando\n");

    if (NT_SUCCESS(CmQueryValue(
            "\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion",
            "ProductName",
            product,
            sizeof(product)))) {
        kputs("[Win32] Registry ProductName: ");
        kputs(product);
        kputs("\n");
    }

  (void)PsGetCurrentProcess();
  (void)SeGetSystemToken();

    kputs("[Win32] GDI/User32 (kernel-mode win32k stub) pronto\n");
    return STATUS_SUCCESS;
}

void Win32SessionManagerStart(void) {
    IRP irp;

    kputs("[Win32] Session Manager: iniciando shell grafica (explorer)\n");

    irp.MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irp.IoControlCode = IOCTL_VIDEO_SET_MODE;
    irp.SystemBuffer = NULL;
    irp.BufferLength = 0;
    IoCallDriverByPath("\\Device\\Video0", &irp);

    irp.IoControlCode = IOCTL_KBD_READ;
    IoCallDriverByPath("\\Device\\KeyboardClass0", &irp);

    irp.IoControlCode = IOCTL_MOUSE_POLL;
    IoCallDriverByPath("\\Device\\PointerClass0", &irp);

    desktop_run();
}
