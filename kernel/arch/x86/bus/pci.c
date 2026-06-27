#include "arch/pci.h"
#include "arch/ports.h"

#define PCI_CONFIG_ADDR 0xCF8u
#define PCI_CONFIG_DATA 0xCFCu

static uint32_t pci_endereco(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    return 0x80000000u
        | ((uint32_t)bus << 16)
        | ((uint32_t)device << 11)
        | ((uint32_t)function << 8)
        | ((uint32_t)offset & 0xFCu);
}

uint32_t pci_config_ler32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    outl(PCI_CONFIG_ADDR, pci_endereco(bus, device, function, offset));
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_config_ler16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t value = pci_config_ler32(bus, device, function, offset);
    if (offset & 2u) {
        return (uint16_t)(value >> 16);
    }
    return (uint16_t)(value & 0xFFFFu);
}

void pci_config_escrever32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    outl(PCI_CONFIG_ADDR, pci_endereco(bus, device, function, offset));
    outl(PCI_CONFIG_DATA, value);
}

uint32_t pci_ler_bar(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index) {
    uint32_t bar = pci_config_ler32(bus, device, function, (uint8_t)(0x10u + bar_index * 4u));
    if ((bar & 0x01u) != 0u) {
        return 0;
    }
    return bar & 0xFFFFFFF0u;
}

uint16_t pci_ler_bar_io(uint8_t bus, uint8_t device, uint8_t function, uint8_t bar_index) {
    uint32_t bar = pci_config_ler32(bus, device, function, (uint8_t)(0x10u + bar_index * 4u));
    if ((bar & 0x01u) == 0u) {
        return 0;
    }
    return (uint16_t)(bar & 0xFFFCu);
}

int pci_encontrar(uint16_t vendor_id, uint16_t device_id, pci_device_t *out) {
    uint8_t bus;
    uint8_t dev;
    uint8_t func;
    uint16_t vendor;

    if (!out) {
        return 0;
    }

    for (bus = 0; bus < 1; ++bus) {
        for (dev = 0; dev < 32; ++dev) {
            for (func = 0; func < 8; ++func) {
                vendor = pci_config_ler16(bus, dev, func, 0);
                if (vendor == 0xFFFFu) {
                    if (func == 0) {
                        break;
                    }
                    continue;
                }
                if (vendor == vendor_id && pci_config_ler16(bus, dev, func, 2) == device_id) {
                    uint32_t cfg = pci_config_ler32(bus, dev, func, 0);
                    uint32_t cls = pci_config_ler32(bus, dev, func, 8);
                    out->bus = bus;
                    out->device = dev;
                    out->function = func;
                    out->vendor_id = vendor;
                    out->device_id = (uint16_t)((cfg >> 16) & 0xFFFFu);
                    out->class_code = (uint8_t)((cls >> 24) & 0xFFu);
                    out->subclass = (uint8_t)((cls >> 16) & 0xFFu);
                    out->prog_if = (uint8_t)((cls >> 8) & 0xFFu);
                    return 1;
                }
            }
        }
    }
    return 0;
}

int pci_encontrar_usb(uint8_t prog_if, pci_device_t *out) {
    uint8_t bus;
    uint8_t dev;
    uint8_t func;
    uint16_t vendor;
    uint32_t cls;

    if (!out) {
        return 0;
    }

    for (bus = 0; bus < 1; ++bus) {
        for (dev = 0; dev < 32; ++dev) {
            for (func = 0; func < 8; ++func) {
                vendor = pci_config_ler16(bus, dev, func, 0);
                if (vendor == 0xFFFFu) {
                    if (func == 0) {
                        break;
                    }
                    continue;
                }

                cls = pci_config_ler32(bus, dev, func, 8);
                if ((uint8_t)((cls >> 24) & 0xFFu) != 0x0Cu) {
                    continue;
                }
                if ((uint8_t)((cls >> 16) & 0xFFu) != 0x03u) {
                    continue;
                }
                if (prog_if != 0xFFu && (uint8_t)((cls >> 8) & 0xFFu) != prog_if) {
                    continue;
                }

                out->bus = bus;
                out->device = dev;
                out->function = func;
                out->vendor_id = vendor;
                out->device_id = pci_config_ler16(bus, dev, func, 2);
                out->class_code = 0x0Cu;
                out->subclass = 0x03u;
                out->prog_if = (uint8_t)((cls >> 8) & 0xFFu);
                return 1;
            }
        }
    }
    return 0;
}

void pci_enumerar(pci_device_t *lista, uint32_t max, uint32_t *count) {
    uint8_t bus;
    uint8_t dev;
    uint8_t func;
    uint32_t n = 0;
    uint32_t cfg;
    uint16_t vendor;

    if (!lista || !count) {
        return;
    }

    *count = 0;

    for (bus = 0; bus < 1; ++bus) {
        for (dev = 0; dev < 32; ++dev) {
            for (func = 0; func < 8; ++func) {
                vendor = pci_config_ler16(bus, dev, func, 0);
                if (vendor == 0xFFFFu) {
                    if (func == 0) {
                        break;
                    }
                    continue;
                }

                if (n >= max) {
                    *count = n;
                    return;
                }

                cfg = pci_config_ler32(bus, dev, func, 0);
                lista[n].bus = bus;
                lista[n].device = dev;
                lista[n].function = func;
                lista[n].vendor_id = (uint16_t)(cfg & 0xFFFFu);
                lista[n].device_id = (uint16_t)((cfg >> 16) & 0xFFFFu);
                cfg = pci_config_ler32(bus, dev, func, 8);
                lista[n].class_code = (uint8_t)((cfg >> 24) & 0xFFu);
                lista[n].subclass = (uint8_t)((cfg >> 16) & 0xFFu);
                lista[n].prog_if = (uint8_t)((cfg >> 8) & 0xFFu);
                ++n;

                if (func == 0) {
                    uint32_t hdr = pci_config_ler32(bus, dev, func, 12);
                    uint8_t header_type = (uint8_t)((hdr >> 16) & 0xFFu);
                    if ((header_type & 0x80u) == 0) {
                        break;
                    }
                }
            }
        }
    }

    *count = n;
}
