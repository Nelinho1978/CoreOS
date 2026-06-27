#include "ntos/io.h"
#include "coreos/printk.h"

#define IO_MAX_DRIVERS 8u
#define IO_MAX_DEVICES 16u

static DRIVER_OBJECT g_drivers[IO_MAX_DRIVERS];
static DEVICE_OBJECT g_devices[IO_MAX_DEVICES];
static ULONG g_driver_count;
static ULONG g_device_count;

NTSTATUS IoCreateDriver(PCSTR name, PDRIVER_DISPATCH dispatch, DRIVER_OBJECT **driver) {
    DRIVER_OBJECT *drv;

    if (!name || !dispatch || !driver || g_driver_count >= IO_MAX_DRIVERS) {
        return STATUS_INVALID_PARAMETER;
    }

    drv = &g_drivers[g_driver_count++];
    drv->Header.Type = ObTypeDevice;
    drv->Header.RefCount = 1;
    {
        SIZE_T i = 0;
        while (name[i] && i + 1 < sizeof(drv->Header.Name)) {
            drv->Header.Name[i] = name[i];
            ++i;
        }
        drv->Header.Name[i] = '\0';
    }
    drv->DriverName = name;
    drv->MajorFunction[IRP_MJ_DEVICE_CONTROL] = dispatch;
    *driver = drv;
    return STATUS_SUCCESS;
}

NTSTATUS IoCreateDevice(DRIVER_OBJECT *driver, PCSTR path, DEVICE_OBJECT **device) {
    DEVICE_OBJECT *dev;

    if (!driver || !path || !device || g_device_count >= IO_MAX_DEVICES) {
        return STATUS_INVALID_PARAMETER;
    }

    dev = &g_devices[g_device_count++];
    dev->Header.Type = ObTypeDevice;
    dev->Header.RefCount = 1;
    {
        SIZE_T i = 0;
        while (path[i] && i + 1 < sizeof(dev->Header.Name)) {
            dev->Header.Name[i] = path[i];
            ++i;
        }
        dev->Header.Name[i] = '\0';
    }
    dev->DriverObject = driver;
    dev->DevicePath = path;
    dev->DeviceIndex = g_device_count;
    *device = dev;
    return STATUS_SUCCESS;
}

DEVICE_OBJECT *IoGetDeviceByPath(PCSTR path) {
    ULONG i;
    if (!path) {
        return NULL;
    }
    for (i = 0; i < g_device_count; ++i) {
        PCSTR a = path;
        PCSTR b = g_devices[i].DevicePath;
        UCHAR match = 1;
        while (*a || *b) {
            if (*a != *b) {
                match = 0;
                break;
            }
            if (*a == '\0') {
                break;
            }
            ++a;
            ++b;
        }
        if (match) {
            return &g_devices[i];
        }
    }
    return NULL;
}

NTSTATUS IoCallDriverByPath(PCSTR path, IRP *irp) {
    DEVICE_OBJECT *dev;
    PDRIVER_DISPATCH dispatch;

    if (!path || !irp) {
        return STATUS_INVALID_PARAMETER;
    }

    dev = IoGetDeviceByPath(path);
    if (!dev || !dev->DriverObject) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    dispatch = dev->DriverObject->MajorFunction[irp->MajorFunction];
    if (!dispatch) {
        dispatch = dev->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];
    }
    if (!dispatch) {
        return STATUS_NOT_IMPLEMENTED;
    }

    irp->Status = dispatch(irp);
    return irp->Status;
}

void IoInitSystem(void) {
    g_driver_count = 0;
    g_device_count = 0;
    kputs("[Io] I/O Manager inicializado\n");
}
