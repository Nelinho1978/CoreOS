#ifndef COREOS_ARCH_X86_OHCI_H
#define COREOS_ARCH_X86_OHCI_H

#include "coreos/usb.h"
#include "coreos/firmware.h"

int ohci_inicializar(const pci_device_t *dev, usb_host_t *host);
int ohci_esta_pronto(void);
int ohci_reset_porta(uint32_t port);
int ohci_get_descriptor(uint8_t addr, uint8_t type, uint8_t index,
                        uint8_t *out, uint16_t maxlen);
int ohci_set_address(uint8_t new_addr);
int ohci_set_configuration(uint8_t addr, uint8_t config);
int ohci_intr_setup(uint8_t addr, uint8_t ep, uint16_t maxpkt);
int ohci_intr_poll(uint8_t *report, uint8_t maxlen);
uint8_t *ohci_data_buffer(void);

#endif
