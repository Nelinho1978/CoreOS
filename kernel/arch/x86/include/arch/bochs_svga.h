#ifndef COREOS_ARCH_X86_BOCHS_SVGA_H
#define COREOS_ARCH_X86_BOCHS_SVGA_H

#include "coreos/types.h"
#include "coreos/firmware.h"

#define BOCHS_VENDOR_ID   0x80EEu
#define BOCHS_DEVICE_VGA  0xBEEFu
#define BOCHS_DEVICE_VBOX 0xCAFEu

#define VBE_DISPI_IOPORT_INDEX 0x01CEu
#define VBE_DISPI_IOPORT_DATA  0x01CFu
#define VBE_DISPI_INDEX_ID         0x00u
#define VBE_DISPI_INDEX_XRES       0x01u
#define VBE_DISPI_INDEX_YRES       0x02u
#define VBE_DISPI_INDEX_BPP        0x03u
#define VBE_DISPI_INDEX_VMEM_SIZE  0x07u
#define VBE_DISPI_ID5              0xB0C5u

typedef struct bochs_dispi_info {
    uint16_t vbe_id;
    uint16_t xres;
    uint16_t yres;
    uint16_t bpp;
    uint32_t vram_kb;
} bochs_dispi_info_t;

int bochs_svga_inicializar(const pci_device_t *gpu);
int bochs_svga_esta_pronto(void);
int bochs_dispi_ler_info(bochs_dispi_info_t *out);
int bochs_svga_fill_rect(int x, int y, int w, int h, uint32_t color);
const char *bochs_svga_nome(void);
void gpu_pci_nome(const pci_device_t *gpu, char *out, size_t out_len);
void gpu_pci_classe(const pci_device_t *gpu, char *out, size_t out_len);
void gpu_pci_fabricante(uint16_t vendor_id, char *out, size_t out_len);

#endif
