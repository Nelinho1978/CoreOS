#ifndef COREOS_NTOS_NTOSKRNL_H
#define COREOS_NTOS_NTOSKRNL_H

#include "nt/ntdef.h"
#include "nt/ntstatus.h"
#include "ntos/ke.h"
#include "ntos/ex.h"
#include "ntos/ob.h"
#include "ntos/mm.h"
#include "ntos/ps.h"
#include "ntos/se.h"
#include "ntos/cm.h"
#include "ntos/io.h"

NTSTATUS NTAPI NtInitializeExecutive(uint32_t boot_magic, void *boot_info);
void NTAPI NtShutdownSystem(void);

#endif
