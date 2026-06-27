#ifndef COREOS_DRIVERS_HW_H
#define COREOS_DRIVERS_HW_H

#include "coreos/types.h"

typedef struct driver_status {
    int video;
    int rede;
    int som;
    int teclado;
    int mouse;
    int usb;
    int display;
    int gpu_accel;
    int opengl;
    int directx;
} driver_status_t;

void drivers_inicializar_hardware(void);
void drivers_marcar_mouse(int ok);
const driver_status_t *drivers_estado(void);
void drivers_imprimir_relatorio(void);

#endif
