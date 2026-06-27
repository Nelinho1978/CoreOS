#ifndef COREOS_NTOS_OB_H
#define COREOS_NTOS_OB_H

#include "nt/ntdef.h"
#include "nt/ntstatus.h"

typedef enum _OBJECT_TYPE {
    ObTypeNone = 0,
    ObTypeDevice,
    ObTypeProcess,
    ObTypeThread,
    ObTypeToken,
    ObTypeKey,
} OBJECT_TYPE;

typedef struct _OBJECT_HEADER {
    OBJECT_TYPE Type;
    ULONG RefCount;
    char Name[64];
} OBJECT_HEADER;

typedef struct _HANDLE_TABLE_ENTRY {
    OBJECT_HEADER *Object;
    ACCESS_MASK GrantedAccess;
} HANDLE_TABLE_ENTRY;

#define OB_MAX_HANDLES 64u

void ObInitSystem(void);
NTSTATUS ObCreateHandle(OBJECT_HEADER *object, ACCESS_MASK access, ULONG *handle);
NTSTATUS ObReferenceObjectByHandle(ULONG handle, OBJECT_TYPE type, OBJECT_HEADER **object);
void ObDereferenceObject(OBJECT_HEADER *object);

#endif
