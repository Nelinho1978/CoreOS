#include "arch/pci.h"
#include "arch/paging.h"
#include "arch/ports.h"
#include "coreos/printk.h"

#define AC97_VENDOR_INTEL 0x8086u
#define AC97_DEV_82801  0x2415u
#define AC97_DEV_MC97   0x7195u

static volatile uint32_t *g_ac97_regs;
static uint16_t g_ac97_mixer_port;
static int g_ac97_ok;

static int ac97_tentar(uint16_t device_id) {
    pci_device_t dev;
    uint32_t bar0;
    uint32_t bar1;
    uint16_t cmd;
    uint16_t mixer_io;

    if (!pci_encontrar(AC97_VENDOR_INTEL, device_id, &dev)) {
        return 0;
    }

    cmd = pci_config_ler16(dev.bus, dev.device, dev.function, 4);
    pci_config_escrever32(dev.bus, dev.device, dev.function, 4, (uint32_t)cmd | 0x05u);

    bar0 = pci_config_ler32(dev.bus, dev.device, dev.function, 0x10);
    bar1 = pci_config_ler32(dev.bus, dev.device, dev.function, 0x14);

    mixer_io = pci_ler_bar_io(dev.bus, dev.device, dev.function, 0);
    if (mixer_io == 0) {
        mixer_io = pci_ler_bar_io(dev.bus, dev.device, dev.function, 1);
    }

    if (mixer_io != 0) {
        g_ac97_mixer_port = mixer_io;
        (void)inw((uint16_t)(mixer_io + 0x2Cu));
        return 1;
    }

    bar0 = pci_ler_bar(dev.bus, dev.device, dev.function, 0);
    if (bar0 == 0) {
        bar0 = pci_ler_bar(dev.bus, dev.device, dev.function, 1);
    }
    if (bar0 == 0) {
        return 0;
    }

    paging_map_mmio(bar0, 4096u);
    g_ac97_regs = (volatile uint32_t *)(uint32_t)bar0;
    g_ac97_regs[0] = 0u;
    (void)bar1;
    return 1;
}

int ac97_inicializar(void) {
    g_ac97_ok = 0;
    g_ac97_regs = NULL;
    g_ac97_mixer_port = 0;

    if (ac97_tentar(AC97_DEV_82801) || ac97_tentar(AC97_DEV_MC97)) {
        g_ac97_ok = 1;
        kputs("[SOM] AC97 detectado e inicializado (VirtualBox)\n");
        return 1;
    }

    kputs("[SOM] AC97 nao encontrado — audio desabilitado na VM?\n");
    return 0;
}

int ac97_esta_pronto(void) {
    return g_ac97_ok;
}
