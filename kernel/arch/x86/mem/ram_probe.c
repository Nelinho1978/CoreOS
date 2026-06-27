#include "arch/ram_probe.h"

#define RAM_PROBE_START 0x00100000u
#define RAM_PROBE_STEP  4096u
#define RAM_PROBE_MAX   (128u * 1024u * 1024u)

uint32_t ram_sondar_limite(void) {
    uint32_t addr;
    volatile uint32_t *ptr;
    uint32_t antigo;
    uint32_t padrao = 0x55AA55AAu;

    for (addr = RAM_PROBE_START; addr < RAM_PROBE_MAX; addr += RAM_PROBE_STEP) {
        ptr = (volatile uint32_t *)(uint32_t)addr;
        antigo = *ptr;
        *ptr = padrao;
        if (*ptr != padrao) {
            if (antigo != padrao) {
                *ptr = antigo;
            }
            return addr;
        }
        *ptr = antigo;
    }

    return RAM_PROBE_MAX;
}
