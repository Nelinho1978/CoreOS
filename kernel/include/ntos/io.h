#ifndef COREOS_NTOS_IO_H
#define COREOS_NTOS_IO_H

#include "nt/ntdef.h"
#include "nt/ntstatus.h"
#include "ntos/ob.h"

typedef enum _IRP_MAJOR {
    IRP_MJ_CREATE = 0,
    IRP_MJ_CLOSE,
    IRP_MJ_READ,
    IRP_MJ_WRITE,
    IRP_MJ_DEVICE_CONTROL,
} IRP_MAJOR;

typedef struct _IRP {
    IRP_MAJOR MajorFunction;
    ULONG IoControlCode;
    PVOID SystemBuffer;
    SIZE_T BufferLength;
    NTSTATUS Status;
} IRP;

typedef NTSTATUS (*PDRIVER_DISPATCH)(IRP *irp);

typedef struct _DRIVER_OBJECT {
    OBJECT_HEADER Header;
    PCSTR DriverName;
    PDRIVER_DISPATCH MajorFunction[8];
} DRIVER_OBJECT;

typedef struct _DEVICE_OBJECT {
    OBJECT_HEADER Header;
    DRIVER_OBJECT *DriverObject;
    PCSTR DevicePath;
    ULONG DeviceIndex;
} DEVICE_OBJECT;

#define IOCTL_VIDEO_SET_MODE  0x230001u
#define IOCTL_KBD_READ        0x230101u
#define IOCTL_MOUSE_POLL        0x230201u

void IoInitSystem(void);
void IoLoadBuiltinDrivers(void);
NTSTATUS IoCreateDriver(PCSTR name, PDRIVER_DISPATCH dispatch, DRIVER_OBJECT **driver);
NTSTATUS IoCreateDevice(DRIVER_OBJECT *driver, PCSTR path, DEVICE_OBJECT **device);
NTSTATUS IoCallDriverByPath(PCSTR path, IRP *irp);
DEVICE_OBJECT *IoGetDeviceByPath(PCSTR path);

#endif
