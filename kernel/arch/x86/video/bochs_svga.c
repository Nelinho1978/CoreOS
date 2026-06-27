#include "arch/bochs_svga.h"
#include "arch/ports.h"
#include "coreos/printk.h"

static int g_bochs_ok;
static char g_bochs_nome[48];

static uint16_t bochs_dispi_read(uint16_t index) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    return inw(VBE_DISPI_IOPORT_DATA);
}

static void str_copy(char *out, size_t out_len, const char *s) {
    size_t i = 0;
    if (!out || out_len == 0) {
        return;
    }
    if (!s) {
        out[0] = '\0';
        return;
    }
    while (s[i] && i + 1 < out_len) {
        out[i] = s[i];
        ++i;
    }
    out[i] = '\0';
}

void gpu_pci_nome(const pci_device_t *gpu, char *out, size_t out_len) {
    if (!gpu || !out || out_len == 0) {
        return;
    }

    if (gpu->vendor_id == BOCHS_VENDOR_ID) {
        if (gpu->device_id == BOCHS_DEVICE_VGA) {
            str_copy(out, out_len, "Bochs VBE (VBoxVGA)");
        } else if (gpu->device_id == BOCHS_DEVICE_VBOX) {
            str_copy(out, out_len, "VirtualBox SVGA II");
        } else {
            str_copy(out, out_len, "Bochs Display Adapter");
        }
    } else if (gpu->vendor_id == 0x8086u) {
        if (gpu->device_id == 0x1237u) {
            str_copy(out, out_len, "Intel 440FX VGA");
        } else {
            str_copy(out, out_len, "Intel Graphics");
        }
    } else if (gpu->vendor_id == 0x10DEu) {
        str_copy(out, out_len, "NVIDIA GeForce / Quadro");
    } else if (gpu->vendor_id == 0x1002u) {
        str_copy(out, out_len, "AMD Radeon Graphics");
    } else if (gpu->vendor_id == 0x15ADu) {
        str_copy(out, out_len, "VMware SVGA II");
    } else if (gpu->vendor_id == 0x1234u) {
        str_copy(out, out_len, "QEMU Standard VGA");
    } else if (gpu->vendor_id == 0x1AF4u) {
        str_copy(out, out_len, "VirtIO GPU");
    } else {
        str_copy(out, out_len, "Adaptador de Video PCI");
    }
}

void gpu_pci_classe(const pci_device_t *gpu, char *out, size_t out_len) {
    if (!gpu || !out || out_len == 0) {
        return;
    }

    switch (gpu->subclass) {
    case 0x00u:
        str_copy(out, out_len, "VGA Compativel");
        break;
    case 0x01u:
        str_copy(out, out_len, "Controlador XGA");
        break;
    case 0x02u:
        str_copy(out, out_len, "Controlador 3D");
        break;
    default:
        str_copy(out, out_len, "Display PCI");
        break;
    }
}

static void gpu_vendor_text(uint16_t vendor_id, char *out, size_t out_len) {
    if (vendor_id == BOCHS_VENDOR_ID) {
        str_copy(out, out_len, "Bochs / VirtualBox");
    } else if (vendor_id == 0x8086u) {
        str_copy(out, out_len, "Intel");
    } else if (vendor_id == 0x10DEu) {
        str_copy(out, out_len, "NVIDIA");
    } else if (vendor_id == 0x1002u) {
        str_copy(out, out_len, "AMD");
    } else if (vendor_id == 0x15ADu) {
        str_copy(out, out_len, "VMware");
    } else if (vendor_id == 0x1234u) {
        str_copy(out, out_len, "QEMU");
    } else {
        str_copy(out, out_len, "PCI Vendor");
    }
}

int bochs_dispi_ler_info(bochs_dispi_info_t *out) {
    uint32_t vmem;

    if (!out) {
        return 0;
    }

    out->vbe_id = bochs_dispi_read(VBE_DISPI_INDEX_ID);
    out->xres = bochs_dispi_read(VBE_DISPI_INDEX_XRES);
    out->yres = bochs_dispi_read(VBE_DISPI_INDEX_YRES);
    out->bpp = bochs_dispi_read(VBE_DISPI_INDEX_BPP);
    vmem = bochs_dispi_read(VBE_DISPI_INDEX_VMEM_SIZE);
    out->vram_kb = vmem * 64u;
    return 1;
}

int bochs_svga_inicializar(const pci_device_t *gpu) {
    bochs_dispi_info_t info;

    g_bochs_ok = 0;
    g_bochs_nome[0] = '\0';

    if (!gpu) {
        return 0;
    }

    gpu_pci_nome(gpu, g_bochs_nome, sizeof(g_bochs_nome));

    if (gpu->vendor_id != BOCHS_VENDOR_ID) {
        return 0;
    }

    if (!bochs_dispi_ler_info(&info)) {
        return 0;
    }

    if (info.vbe_id != VBE_DISPI_ID5 && (info.vbe_id & 0xFFF0u) != 0xB0C0u) {
        kputs("[GPU] Bochs VBE ID invalido: ");
        kprint_hex(info.vbe_id);
        kputs("\n");
        return 0;
    }

    g_bochs_ok = 1;
    kputs("[GPU] Bochs SVGA OK — ");
    kputs(g_bochs_nome);
    kputs(" (VBE 0x");
    kprint_hex(info.vbe_id);
    kputs(" VRAM=");
    kprintf("%u", info.vram_kb);
    kputs("KB)\n");
    return 1;
}

int bochs_svga_esta_pronto(void) {
    return g_bochs_ok;
}

int bochs_svga_fill_rect(int x, int y, int w, int h, uint32_t color) {
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)color;
    return 0;
}

const char *bochs_svga_nome(void) {
    return g_bochs_nome;
}

void gpu_pci_fabricante(uint16_t vendor_id, char *out, size_t out_len) {
    gpu_vendor_text(vendor_id, out, out_len);
}
