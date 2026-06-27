#ifndef COREOS_NT_NTDEF_H
#define COREOS_NT_NTDEF_H

#include "coreos/types.h"

typedef int32_t NTSTATUS;
typedef void *PVOID;
typedef const void *PCVOID;
typedef char *PCHAR;
typedef const char *PCSTR;
typedef uint32_t ULONG;
typedef uint16_t USHORT;
typedef uint8_t UCHAR;
typedef size_t SIZE_T;
typedef uint32_t ACCESS_MASK;

#define NTAPI
#define IN
#define OUT

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#define NT_INFORMATION(Status) ((ULONG)(Status) >> 30 == 1)
#define NT_WARNING(Status) ((ULONG)(Status) >> 30 == 2)
#define NT_ERROR(Status) ((ULONG)(Status) >> 30 == 3)

#define OBJ_INHERIT             0x00000002u
#define OBJ_KERNEL_HANDLE       0x00000200u

#endif
