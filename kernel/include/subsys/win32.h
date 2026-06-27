#ifndef COREOS_SUBSYS_WIN32_H
#define COREOS_SUBSYS_WIN32_H

#include "nt/ntdef.h"

NTSTATUS Win32SubsystemInitialize(void);
void Win32SessionManagerStart(void);

#endif
