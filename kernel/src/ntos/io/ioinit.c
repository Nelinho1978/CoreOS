#include "ntos/io.h"
#include "coreos/printk.h"
#include "arch/irq.h"

/* Global device/driver registry */
#define MAX_DRIVERS 32
#define MAX_DEVICES 64

static DRIVER_OBJECT *g_DriverList = NULL;
static DEVICE_OBJECT *g_DeviceList = NULL;
static int g_DriverCount = 0;
static int g_DeviceCount = 0;

static DRIVER_OBJECT g_DriverTable[MAX_DRIVERS];
static DEVICE_OBJECT g_DeviceTable[MAX_DEVICES];

void IoInitSystem(void) {
    kputs("[Io] I/O Manager initializing...\n");
    g_DriverCount = 0;
    g_DeviceCount = 0;
    kputs("[Io] I/O Manager ready\n");
}

NTSTATUS IoCreateDriver(PCSTR name, PDRIVER_DISPATCH dispatch, DRIVER_OBJECT **driver) {
    if (!name || !driver || g_DriverCount >= MAX_DRIVERS)
        return STATUS_NO_MEMORY;
    
    DRIVER_OBJECT *drv = &g_DriverTable[g_DriverCount];
    drv->DriverName = name;
    drv->MajorFunction[IRP_MJ_CREATE] = dispatch;
    drv->MajorFunction[IRP_MJ_READ] = dispatch;
    drv->MajorFunction[IRP_MJ_WRITE] = dispatch;
    
    *driver = drv;
    g_DriverCount++;
    
    kprintf("[Io] Driver created: %s\n", name);
    return STATUS_SUCCESS;
}

NTSTATUS IoCreateDevice(DRIVER_OBJECT *driver, PCSTR path, DEVICE_OBJECT **device) {
    if (!driver || !path || !device || g_DeviceCount >= MAX_DEVICES)
        return STATUS_NO_MEMORY;
    
    DEVICE_OBJECT *dev = &g_DeviceTable[g_DeviceCount];
    dev->DriverObject = driver;
    dev->DevicePath = path;
    dev->DeviceIndex = g_DeviceCount;
    
    /* Link to device list */
    if (g_DeviceList) {
        DEVICE_OBJECT *d = g_DeviceList;
        while (d->DriverObject) d++;  /* Find next free slot */
        *d = *dev;
    } else {
        g_DeviceList = dev;
    }
    
    *device = dev;
    g_DeviceCount++;
    
    kprintf("[Io] Device created: %s\n", path);
    return STATUS_SUCCESS;
}

DEVICE_OBJECT *IoGetDeviceByPath(PCSTR path) {
    int i;
    for (i = 0; i < g_DeviceCount; ++i) {
        if (g_DeviceTable[i].DevicePath) {
            const char *a = g_DeviceTable[i].DevicePath;
            const char *b = path;
            int match = 1;
            while (*a && *b) {
                if (*a != *b) { match = 0; break; }
                ++a; ++b;
            }
            if (match && *a == *b)
                return &g_DeviceTable[i];
        }
    }
    return NULL;
}

NTSTATUS IoCallDriverByPath(PCSTR path, IRP *irp) {
    DEVICE_OBJECT *dev = IoGetDeviceByPath(path);
    if (!dev || !dev->DriverObject)
        return STATUS_DEVICE_NOT_READY;
    
    DRIVER_OBJECT *drv = dev->DriverObject;
    int major = irp->MajorFunction;
    
    if (major >= 8 || !drv->MajorFunction[major])
        return STATUS_NOT_IMPLEMENTED;
    
    return drv->MajorFunction[major](irp);
}

void IoLoadBuiltinDrivers(void) {
    kputs("[Io] Loading built-in drivers...\n");
    
    /* Video driver */
    extern NTSTATUS video_driver_dispatch(IRP *irp);
    DRIVER_OBJECT *video_drv = NULL;
    IoCreateDriver("\\\\Driver\\\\Video", video_driver_dispatch, &video_drv);
    DEVICE_OBJECT *video_dev = NULL;
    IoCreateDevice(video_drv, "\\\\Device\\\\Video0", &video_dev);
    
    /* Keyboard driver */
    extern NTSTATUS keyboard_driver_dispatch(IRP *irp);
    DRIVER_OBJECT *kbd_drv = NULL;
    IoCreateDriver("\\\\Driver\\\\Keyboard", keyboard_driver_dispatch, &kbd_drv);
    DEVICE_OBJECT *kbd_dev = NULL;
    IoCreateDevice(kbd_drv, "\\\\Device\\\\KeyboardClass0", &kbd_dev);
    
    /* Mouse driver */
    extern NTSTATUS mouse_driver_dispatch(IRP *irp);
    DRIVER_OBJECT *mouse_drv = NULL;
    IoCreateDriver("\\\\Driver\\\\Mouse", mouse_driver_dispatch, &mouse_drv);
    DEVICE_OBJECT *mouse_dev = NULL;
    IoCreateDevice(mouse_drv, "\\\\Device\\\\PointerClass0", &mouse_dev);
    
    kputs("[Io] Built-in drivers loaded\n");
}
