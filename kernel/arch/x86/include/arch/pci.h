#ifndef COREOS_ARCH_X86_PCI_H
#define COREOS_ARCH_X86_PCI_H

#include "coreos/types.h"
#include "coreos/firmware.h"

uint32_t pci_config_ler32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_config_escrever32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
uint16_t pci_config_ler16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint32_t pci_ler_bar(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index);
uint16_t pci_ler_bar_io(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index);
int pci_encontrar(uint16_t vendor_id, uint16_t device_id, pci_device_t *out);
int pci_encontrar_usb(uint8_t prog_if, pci_device_t *out);
void pci_enumerar(pci_device_t *lista, uint32_t max, uint32_t *count);

#endif
