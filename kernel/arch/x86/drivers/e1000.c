#include "arch/pci.h"
#include "arch/paging.h"
#include "coreos/printk.h"

#define E1000_VENDOR 0x8086u
#define E1000_DEVICE 0x100Eu

#define E1000_REG_CTRL   0x0000u
#define E1000_REG_STATUS 0x0008u
#define E1000_CTRL_RST   (1u << 26)
#define E1000_STATUS_LU  (1u << 1)

static volatile uint32_t *g_e1000_regs;
static int g_e1000_ok;

int e1000_inicializar(void) {
    pci_device_t dev;
    uint32_t bar;
    uint16_t cmd;

    g_e1000_ok = 0;
    g_e1000_regs = NULL;

    if (!pci_encontrar(E1000_VENDOR, E1000_DEVICE, &dev)) {
        kputs("[REDE] Intel 82540EM nao encontrado no PCI\n");
        return 0;
    }

    cmd = pci_config_ler16(dev.bus, dev.device, dev.function, 4);
    pci_config_escrever32(dev.bus, dev.device, dev.function, 4, (uint32_t)cmd | 0x06u);

    bar = pci_ler_bar(dev.bus, dev.device, dev.function, 0);
    if (bar == 0) {
        kputs("[REDE] BAR0 invalido\n");
        return 0;
    }

    paging_map_mmio(bar, 128u * 1024u);
    g_e1000_regs = (volatile uint32_t *)(uint32_t)bar;

    g_e1000_regs[E1000_REG_CTRL / 4u] = E1000_CTRL_RST;
    for (uint32_t i = 0; i < 100000u; ++i) {
        if ((g_e1000_regs[E1000_REG_CTRL / 4u] & E1000_CTRL_RST) == 0u) {
            break;
        }
        __asm__ volatile("pause");
    }

    if (g_e1000_regs[E1000_REG_STATUS / 4u] & E1000_STATUS_LU) {
        kputs("[REDE] Intel 82540EM OK — link ativo\n");
    } else {
        kputs("[REDE] Intel 82540EM OK — chip resetado (link pendente)\n");
    }

    g_e1000_ok = 1;
    return 1;
}

int e1000_esta_pronto(void) {
    return g_e1000_ok;
}
