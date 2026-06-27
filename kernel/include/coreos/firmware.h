#ifndef COREOS_FIRMWARE_H
#define COREOS_FIRMWARE_H

#include "coreos/types.h"

#define FIRMWARE_MAX_PCI 32u
#define FIRMWARE_CPU_VENDOR_LEN 16u
#define FIRMWARE_CPU_BRAND_LEN 48u

typedef struct cpu_info {
    char vendor[FIRMWARE_CPU_VENDOR_LEN];
    char brand[FIRMWARE_CPU_BRAND_LEN];
    uint32_t family;
    uint32_t model;
    uint32_t stepping;
    uint32_t features_edx;
    uint32_t features_ecx;
} cpu_info_t;

typedef struct pci_device {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
} pci_device_t;

typedef struct firmware_state {
    cpu_info_t cpu;
    uint32_t ram_bytes;
    pci_device_t pci[FIRMWARE_MAX_PCI];
    uint32_t pci_count;
    int video_ok;
    uint32_t fb_phys;
    uint32_t fb_width;
    uint32_t fb_height;
} firmware_state_t;

void firmware_iniciar(void);
const firmware_state_t *firmware_estado(void);

void inicializar_cpu(void);
void inicializar_memoria(void);
void inicializar_pci(void);
void inicializar_video(void);

#endif
