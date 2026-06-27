#include "arch/pit.h"
#include "arch/ports.h"

#define PIT_CH0 0x40u
#define PIT_CMD 0x43u
#define PIT_TICKS_PER_MS 1193u  /* 1193182 Hz / 1000 */

void pit_init(void) {
    /* Nao reprogramar o PIT — evita IRQ 0 mais frequente antes do IDT existir */
}

uint16_t pit_read_ch0(void) {
    outb(PIT_CMD, 0x00u);
    {
        uint8_t lo = inb(PIT_CH0);
        uint8_t hi = inb(PIT_CH0);
        return (uint16_t)((uint16_t)hi << 8) | lo;
    }
}

void pit_accumulate_ms(uint16_t *last, uint32_t *elapsed_ms, uint32_t *rem_ticks) {
    uint16_t now = pit_read_ch0();
    uint16_t delta;

    if (now <= *last) {
        delta = (uint16_t)(*last - now);
    } else {
        delta = (uint16_t)(*last + (uint16_t)(65535u - now));
    }

    *last = now;
    *rem_ticks += (uint32_t)delta;

    while (*rem_ticks >= PIT_TICKS_PER_MS) {
        *rem_ticks -= PIT_TICKS_PER_MS;
        (*elapsed_ms)++;
    }
}
