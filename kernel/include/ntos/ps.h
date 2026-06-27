#ifndef COREOS_NTOS_PS_H
#define COREOS_NTOS_PS_H

#include "nt/ntdef.h"
#include "ntos/ob.h"

typedef struct _KTHREAD {
    OBJECT_HEADER Header;
    ULONG ThreadId;
    ULONG Priority;
} KTHREAD;

typedef struct _EPROCESS {
    OBJECT_HEADER Header;
    ULONG ProcessId;
    PCSTR ImageFileName;
    KTHREAD *PrimaryThread;
} EPROCESS;

void PspInitSystem(void);
EPROCESS *PsGetCurrentProcess(void);
EPROCESS *PsGetInitialSystemProcess(void);

#endif
