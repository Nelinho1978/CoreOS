#include "coreos/boot_mode.h"
#include "arch/keyboard.h"
#include "arch/vga.h"

#define MENU_TIMEOUT_SEC 5u
/* ~5 s no VirtualBox (polling, sem depender do PIT) */
#define LOOPS_PER_SEC    3000000u

#define COLOR_BG 0x01u
#define COLOR_FG 0x0Fu
#define COLOR_SEL_FG 0x00u
#define COLOR_SEL_BG 0x0Fu
#define COLOR_DIM 0x0Bu

static const char *const MENU_ITEMS[] = {
    "Instalar CoreOS 10 no computador",
    "Executar CoreOS 10 (modo Live)",
};

#define MENU_COUNT 2u

static void draw_menu(uint32_t selected, uint32_t seconds_left) {
    char countdown[56];
    size_t pos = 0;

    x86_vga_fill_screen(COLOR_FG, COLOR_BG);

    x86_vga_write_at(2, 28, "CoreOS 10", COLOR_FG, COLOR_BG);
    x86_vga_fill_span(3, 28, 9, '=', COLOR_FG, COLOR_BG);
    x86_vga_write_at(5, 22, "Bem-vindo ao assistente de inicializacao", COLOR_FG, COLOR_BG);
    x86_vga_write_at(7, 20, "O que deseja fazer?", COLOR_FG, COLOR_BG);

    for (uint32_t i = 0; i < MENU_COUNT; ++i) {
        size_t row = 10 + i * 2;
        uint8_t fg = COLOR_FG;
        uint8_t bg = COLOR_BG;
        const char *prefix = "  ";

        if (i == selected) {
            fg = COLOR_SEL_FG;
            bg = COLOR_SEL_BG;
            prefix = "> ";
        }

        x86_vga_fill_span(row, 16, 48, ' ', fg, bg);
        x86_vga_write_at(row, 16, prefix, fg, bg);
        x86_vga_write_at(row, 19, MENU_ITEMS[i], fg, bg);
    }

    x86_vga_write_at(15, 14, "Setas ou 1/2 para escolher   Enter para confirmar", COLOR_DIM, COLOR_BG);
    x86_vga_write_at(16, 18, "Esc = modo Live imediato", COLOR_DIM, COLOR_BG);

    x86_vga_fill_span(22, 0, 80, ' ', COLOR_DIM, COLOR_BG);

    {
        const char *prefix = "Iniciando modo Live em ";
        const char *suffix = "s...";
        for (const char *p = prefix; *p; ++p) {
            countdown[pos++] = *p;
        }
        if (seconds_left >= 10) {
            countdown[pos++] = (char)('0' + (seconds_left / 10));
        }
        countdown[pos++] = (char)('0' + (seconds_left % 10));
        for (const char *p = suffix; *p; ++p) {
            countdown[pos++] = *p;
        }
        countdown[pos] = '\0';
    }
    x86_vga_write_at(22, 10, countdown, COLOR_DIM, COLOR_BG);
}

boot_mode_t boot_menu_show(void) {
    uint32_t selected = 0;
    uint32_t seconds_left = MENU_TIMEOUT_SEC;
    uint32_t loops = 0;

    x86_keyboard_init();
    draw_menu(selected, seconds_left);

    for (;;) {
        key_event_t key = x86_keyboard_poll();

        if (key == KEY_UP && selected > 0) {
            --selected;
            draw_menu(selected, seconds_left);
        } else if (key == KEY_DOWN && selected + 1 < MENU_COUNT) {
            ++selected;
            draw_menu(selected, seconds_left);
        } else if (key == KEY_1) {
            selected = 0;
            draw_menu(selected, seconds_left);
        } else if (key == KEY_2) {
            selected = 1;
            draw_menu(selected, seconds_left);
        } else if (key == KEY_ENTER) {
            return selected == 0 ? BOOT_MODE_INSTALL : BOOT_MODE_LIVE;
        } else if (key == KEY_ESC) {
            return BOOT_MODE_LIVE;
        }

        ++loops;

        if (loops >= LOOPS_PER_SEC * MENU_TIMEOUT_SEC) {
            return BOOT_MODE_LIVE;
        }

        {
            uint32_t elapsed_sec = loops / LOOPS_PER_SEC;
            uint32_t new_seconds = MENU_TIMEOUT_SEC - elapsed_sec;
            if (new_seconds > MENU_TIMEOUT_SEC) {
                new_seconds = 0;
            }
            if (new_seconds != seconds_left) {
                seconds_left = new_seconds;
                draw_menu(selected, seconds_left);
            }
        }

        __asm__ volatile("pause");
    }
}
