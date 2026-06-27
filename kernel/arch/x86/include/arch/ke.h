#ifndef COREOS_ARCH_KE_H
#define COREOS_ARCH_KE_H

#include "coreos/types.h"

void KeArchInitSystem(void);
void KeArchPhase1Init(void);
void KeLoadGdt(void);
void KeLoadIdt(void);
void KeEnableSyscalls(void);

#endif
