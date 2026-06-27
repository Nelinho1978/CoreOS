#ifndef COREOS_NTOS_KE_H
#define COREOS_NTOS_KE_H

#include "nt/ntdef.h"

void KeInitSystem(void);
void KePhase1Init(void);
void KeIrqDispatch(uint32_t vector);

#endif
