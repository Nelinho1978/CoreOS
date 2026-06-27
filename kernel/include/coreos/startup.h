#ifndef COREOS_STARTUP_H
#define COREOS_STARTUP_H

#include "coreos/types.h"

void kernel_executar(uint32_t boot_magic, void *boot_info);

void criar_memoria_virtual(uint32_t boot_magic, void *boot_info);
void criar_scheduler(void);
void criar_drivers(void);
void criar_processos(void);
void mostrar_tela(void);

#endif
