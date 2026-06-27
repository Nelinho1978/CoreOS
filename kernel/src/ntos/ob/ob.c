#include "ntos/ob.h"
#include "coreos/printk.h"

static HANDLE_TABLE_ENTRY g_handles[OB_MAX_HANDLES];

void ObInitSystem(void) {
    ULONG i;
    for (i = 0; i < OB_MAX_HANDLES; ++i) {
        g_handles[i].Object = NULL;
        g_handles[i].GrantedAccess = 0;
    }
    kputs("[Ob] Object Manager inicializado\n");
}

NTSTATUS ObCreateHandle(OBJECT_HEADER *object, ACCESS_MASK access, ULONG *handle) {
    ULONG i;

    if (!object || !handle) {
        return STATUS_INVALID_PARAMETER;
    }

    for (i = 1; i < OB_MAX_HANDLES; ++i) {
        if (g_handles[i].Object == NULL) {
            object->RefCount++;
            g_handles[i].Object = object;
            g_handles[i].GrantedAccess = access;
            *handle = i;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NO_MEMORY;
}

NTSTATUS ObReferenceObjectByHandle(ULONG handle, OBJECT_TYPE type, OBJECT_HEADER **object) {
    if (handle == 0 || handle >= OB_MAX_HANDLES || !object) {
        return STATUS_INVALID_PARAMETER;
    }
    if (!g_handles[handle].Object) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    if (type != ObTypeNone && g_handles[handle].Object->Type != type) {
        return STATUS_INVALID_PARAMETER;
    }
    *object = g_handles[handle].Object;
    return STATUS_SUCCESS;
}

void ObDereferenceObject(OBJECT_HEADER *object) {
    if (!object || object->RefCount == 0) {
        return;
    }
    object->RefCount--;
}
