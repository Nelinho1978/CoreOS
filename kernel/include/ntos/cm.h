#ifndef COREOS_NTOS_CM_H
#define COREOS_NTOS_CM_H

#include "nt/ntdef.h"
#include "nt/ntstatus.h"

#define CM_MAX_KEYS 32u
#define CM_VALUE_LEN  128u
#define CM_PATH_LEN   96u

void CmInitSystem(void);
NTSTATUS CmSetValue(PCSTR path, PCSTR name, PCSTR value);
NTSTATUS CmQueryValue(PCSTR path, PCSTR name, PCHAR buffer, SIZE_T buffer_len);

#endif
