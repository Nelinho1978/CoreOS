#include "coreos/boot_mode.h"
#include "coreos/printk.h"
#include "hal/hal.h"
#include "arch/vga.h"

static void delay_step(uint32_t loops) {
    for (uint32_t i = 0; i < loops; ++i) {
        if (hal_get()->cpu_pause) {
            hal_get()->cpu_pause();
        }
    }
}

static void draw_progress(uint32_t percent) {
    char bar[42];
    uint32_t filled = (percent * 30u) / 100u;
    size_t i = 0;

    bar[i++] = '[';
    for (uint32_t p = 0; p < 30; ++p) {
        bar[i++] = (p < filled) ? '#' : ' ';
    }
    bar[i++] = ']';
    bar[i++] = ' ';
    if (percent >= 100) {
        bar[i++] = '1';
        bar[i++] = '0';
        bar[i++] = '0';
    } else if (percent >= 10) {
        bar[i++] = (char)('0' + (percent / 10));
        bar[i++] = (char)('0' + (percent % 10));
    } else {
        bar[i++] = (char)('0' + percent);
    }
    bar[i++] = '%';
    bar[i] = '\0';

    x86_vga_write_at(12, 20, bar, 0x0F, 0x01);
}

void setup_run_installer(void) {
    const char *steps[] = {
        "Verificando disco de destino...",
        "Criando particao do sistema...",
        "Formatando unidade C: (CoreOS)...",
        "Copiando arquivos do sistema...",
        "Instalando componentes do Windows 10...",
        "Configurando boot e registro...",
        "Finalizando instalacao...",
    };
    const uint32_t step_count = sizeof(steps) / sizeof(steps[0]);

    hal_clear_screen();
    hal_get()->console_set_color(0x0F, 0x01);
    kputs("\n  CoreOS 10 Setup\n");
    kputs("  ===============\n\n");
    kputs("  Preparando instalacao no disco...\n\n");

    for (uint32_t s = 0; s < step_count; ++s) {
        kputs("  ");
        kputs(steps[s]);
        kputc('\n');

        for (uint32_t p = 0; p <= 100; p += 5) {
            uint32_t overall = ((s * 100u) + p) / step_count;
            draw_progress(overall);
            delay_step(120000);
        }
    }

    draw_progress(100);
    kputs("\n  Instalacao concluida!\n");
    kputs("  Remova o disco de instalacao e reinicie o computador.\n");
    kputs("  (Instalador real em desenvolvimento — esta e uma simulacao.)\n\n");

    for (;;) {
        if (hal_get()->cpu_halt) {
            hal_get()->cpu_halt();
        }
    }
}
