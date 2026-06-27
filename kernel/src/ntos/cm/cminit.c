#include "ntos/cm.h"
#include "coreos/printk.h"

typedef struct _CM_ENTRY {
    char Path[CM_PATH_LEN];
    char Name[32];
    char Value[CM_VALUE_LEN];
    UCHAR Used;
} CM_ENTRY;

static CM_ENTRY g_registry[CM_MAX_KEYS];

static void cm_copy_str(char *dst, SIZE_T dst_len, PCSTR src) {
    SIZE_T i = 0;
    if (!dst || dst_len == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    while (src[i] && i + 1 < dst_len) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

void CmInitSystem(void) {
    ULONG i;
    for (i = 0; i < CM_MAX_KEYS; ++i) {
        g_registry[i].Used = 0;
        g_registry[i].Path[0] = '\0';
        g_registry[i].Name[0] = '\0';
        g_registry[i].Value[0] = '\0';
    }

    CmSetValue("\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion",
               "ProductName", "CoreOS 10");
    CmSetValue("\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion",
               "CurrentVersion", "10.0");
    CmSetValue("\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion",
               "CurrentBuild", "19045");
    CmSetValue("\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager",
               "BootExecute", "Win32Subsys");

    kputs("[Cm] Configuration Manager (Registry) inicializado\n");
}

NTSTATUS CmSetValue(PCSTR path, PCSTR name, PCSTR value) {
    ULONG i;

    if (!path || !name || !value) {
        return STATUS_INVALID_PARAMETER;
    }

    for (i = 0; i < CM_MAX_KEYS; ++i) {
        if (g_registry[i].Used &&
            g_registry[i].Path[0] == path[0]) {
            SIZE_T pi = 0;
            UCHAR match = 1;
            while (path[pi] && g_registry[i].Path[pi]) {
                if (path[pi] != g_registry[i].Path[pi]) {
                    match = 0;
                    break;
                }
                ++pi;
            }
            if (match && path[pi] == '\0' && g_registry[i].Path[pi] == '\0') {
                SIZE_T ni = 0;
                while (name[ni] && g_registry[i].Name[ni] && name[ni] == g_registry[i].Name[ni]) {
                    ++ni;
                }
                if (name[ni] == '\0' && g_registry[i].Name[ni] == '\0') {
                    cm_copy_str(g_registry[i].Value, CM_VALUE_LEN, value);
                    return STATUS_SUCCESS;
                }
            }
        }
    }

    for (i = 0; i < CM_MAX_KEYS; ++i) {
        if (!g_registry[i].Used) {
            g_registry[i].Used = 1;
            cm_copy_str(g_registry[i].Path, CM_PATH_LEN, path);
            cm_copy_str(g_registry[i].Name, 32, name);
            cm_copy_str(g_registry[i].Value, CM_VALUE_LEN, value);
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NO_MEMORY;
}

NTSTATUS CmQueryValue(PCSTR path, PCSTR name, PCHAR buffer, SIZE_T buffer_len) {
    ULONG i;

    if (!path || !name || !buffer || buffer_len == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    for (i = 0; i < CM_MAX_KEYS; ++i) {
        if (!g_registry[i].Used) {
            continue;
        }
        {
            SIZE_T pi = 0;
            UCHAR match = 1;
            while (path[pi] || g_registry[i].Path[pi]) {
                if (path[pi] != g_registry[i].Path[pi]) {
                    match = 0;
                    break;
                }
                if (path[pi] == '\0') {
                    break;
                }
                ++pi;
            }
            if (!match) {
                continue;
            }
        }
        {
            SIZE_T ni = 0;
            while (name[ni] || g_registry[i].Name[ni]) {
                if (name[ni] != g_registry[i].Name[ni]) {
                    goto next_entry;
                }
                if (name[ni] == '\0') {
                    break;
                }
                ++ni;
            }
        }
        cm_copy_str(buffer, buffer_len, g_registry[i].Value);
        return STATUS_SUCCESS;
next_entry:
        ;
    }

    return STATUS_OBJECT_NAME_NOT_FOUND;
}
