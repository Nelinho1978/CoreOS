#include "gui/win10.h"
#include "gui/fb.h"
#include "gfx/gfx_accel.h"
#include "gfx/opengl.h"
#include "gfx/directx.h"
#include "gfx/display.h"
#include "coreos/usb.h"
#include "arch/keyboard.h"
#include "arch/mouse.h"
#include "arch/vga.h"
#include "coreos/drivers_hw.h"

#define MAX_WINS 5
#define MAX_ICONS 5

typedef enum {
    APP_NONE = 0,
    APP_TERMINAL,
    APP_EXPLORER,
    APP_ABOUT,
    APP_SETTINGS,
} app_type_t;

typedef struct {
    int x, y, w, h;
    app_type_t type;
    int open;
    int active;
    int minimized;
    const char *title;
} gui_window_t;

typedef struct {
    int x, y;
    app_type_t type;
    const char *label;
    const char *glyph;
} desktop_icon_t;

static gui_window_t g_wins[MAX_WINS];
static int g_win_count;
static int g_start_open;
static int g_drag_win = -1;
static int g_drag_off_x;
static int g_drag_off_y;
static uint32_t g_tick;
static int g_dirty = 1;

static desktop_icon_t g_icons[MAX_ICONS] = {
    {24, 40, APP_TERMINAL, "Prompt", ">_"},
    {24, 120, APP_EXPLORER, "PC", "PC"},
    {24, 200, APP_SETTINGS, "Cfg", "[]"},
    {24, 280, APP_ABOUT, "Sobre", "i"},
    {24, 360, APP_NONE, "Lix", "X"},
};

static void draw_outset(int x, int y, int w, int h) {
    fb_fill_rect(x, y, w, h, W95_FACE);
    fb_hline(x, y, w, W95_HIGHLIGHT);
    fb_vline(x, y, h, W95_HIGHLIGHT);
    fb_hline(x + 1, y + h - 2, w - 2, W95_SHADOW);
    fb_vline(x + w - 2, y + 1, h - 2, W95_SHADOW);
    fb_hline(x, y + h - 1, w, W95_SHADOW_DARK);
    fb_vline(x + w - 1, y, h, W95_SHADOW_DARK);
}

static void draw_inset(int x, int y, int w, int h) {
    fb_fill_rect(x, y, w, h, W95_FACE);
    fb_hline(x, y, w, W95_SHADOW);
    fb_vline(x, y, h, W95_SHADOW);
    fb_hline(x + 1, y + h - 2, w - 2, W95_HIGHLIGHT);
    fb_vline(x + w - 2, y + 1, h - 2, W95_HIGHLIGHT);
}

static void draw_title_bar(int x, int y, int w, const char *title, int active) {
    int i;
    uint32_t c1 = active ? W95_TITLE_A : W95_FACE_DARK;
    uint32_t c2 = active ? W95_TITLE_B : W95_FACE_LIGHT;
    for (i = 0; i < w; ++i) {
        uint32_t c = (i < w / 2) ? c1 : c2;
        fb_hline(x + i, y, 1, c);
        fb_hline(x + i, y + 1, 1, c);
    }
    fb_draw_string(x + 6, y + 4, title, active ? W95_TITLE_TEXT : W95_TEXT, c1);
    draw_outset(x + w - 52, y + 3, 20, 16);
    fb_draw_string(x + w - 46, y + 4, "_", W95_TEXT, W95_FACE);
    draw_outset(x + w - 30, y + 3, 20, 16);
    fb_draw_string(x + w - 24, y + 4, "x", W95_TEXT, W95_FACE);
}

static void draw_gfx_logo(int ox, int oy) {
    d3d_device_t *dev;
    d3d_vertex_t tri[3];

    if (!d3d_esta_pronto()) {
        return;
    }

    dev = d3d_create_device();
    if (!dev) {
        return;
    }

    tri[0].x = ox;
    tri[0].y = oy + 40;
    tri[0].color = 0x0078D7u;
    tri[1].x = ox + 40;
    tri[1].y = oy + 40;
    tri[1].color = 0x00FFFFFFu;
    tri[2].x = ox + 20;
    tri[2].y = oy;
    tri[2].color = 0x00FFB900u;
    d3d_device_draw_primitive(dev, D3DPT_TRIANGLELIST, tri, 3);
}

static void draw_wallpaper(void) {
    const framebuffer_t *fb = fb_get();
    int cx;
    int x;
    int y;

    fb_fill_rect(0, 0, (int)fb->width, (int)fb->height - (int)W95_TASKBAR_H, W95_DESKTOP);

    struct { int cx, cy, r; } clouds[] = {
        {120, 80, 50}, {300, 60, 60}, {520, 100, 55},
        {680, 70, 65}, {200, 180, 45}, {450, 200, 58},
    };
    for (cx = 0; cx < 6; ++cx) {
        for (y = -clouds[cx].r; y <= clouds[cx].r; ++y) {
            for (x = -clouds[cx].r; x <= clouds[cx].r; ++x) {
                if (x * x + y * y <= clouds[cx].r * clouds[cx].r) {
                    fb_put_pixel(clouds[cx].cx + x, clouds[cx].cy + y, W95_CLOUD);
                }
            }
        }
    }

    if (gl_esta_pronto()) {
        glViewport((int)fb->width - 120, 24, 80, 60);
        glColor4ub(0, 120, 215, 255);
        glBegin(GL_TRIANGLES);
        glVertex2i((int)fb->width - 110, 74);
        glVertex2i((int)fb->width - 70, 74);
        glVertex2i((int)fb->width - 90, 34);
        glEnd();
    } else {
        draw_gfx_logo((int)fb->width - 110, 34);
    }
}

static gui_window_t *win_find(app_type_t type) {
    int i;
    for (i = 0; i < g_win_count; ++i) {
        if (g_wins[i].type == type && g_wins[i].open) {
            return &g_wins[i];
        }
    }
    return NULL;
}

static gui_window_t *win_create(app_type_t type, const char *title, int x, int y, int w, int h) {
    gui_window_t *wptr = win_find(type);
    if (wptr) {
        wptr->minimized = 0;
        wptr->active = 1;
        return wptr;
    }
    if (g_win_count >= MAX_WINS) {
        return NULL;
    }
    wptr = &g_wins[g_win_count++];
    wptr->x = x;
    wptr->y = y;
    wptr->w = w;
    wptr->h = h;
    wptr->type = type;
    wptr->open = 1;
    wptr->active = 1;
    wptr->minimized = 0;
    wptr->title = title;
    return wptr;
}

static void win_close(gui_window_t *w) {
    w->open = 0;
    w->active = 0;
}

static void draw_display_info(int cx, int cy) {
    const gpu_adapter_info_t *gpu = display_gpu();
    const monitor_info_t *mon = display_monitor();
    char resbuf[24];

    if (!gpu || !mon) {
        return;
    }

    fb_draw_string(cx + 48, cy + 68, gpu->name, W95_TEXT, W95_FACE);
    fb_draw_string(cx + 48, cy + 84, gpu->chip_class, W95_TEXT, W95_FACE);
    fb_draw_string(cx + 48, cy + 100, mon->name, W95_TEXT, W95_FACE);

    display_format_resolucao(resbuf, sizeof(resbuf),
                             mon->current_width, mon->current_height, mon->current_bpp);
    fb_draw_string(cx + 48, cy + 116, "Atual:", W95_TEXT, W95_FACE);
    fb_draw_string(cx + 104, cy + 116, resbuf, W95_TEXT, W95_FACE);

    display_format_resolucao(resbuf, sizeof(resbuf),
                             mon->max_width, mon->max_height, 0);
    fb_draw_string(cx + 48, cy + 132, "Max:", W95_TEXT, W95_FACE);
    fb_draw_string(cx + 104, cy + 132, resbuf, W95_TEXT, W95_FACE);
}

static void draw_window_content(gui_window_t *w) {
    int cx = w->x + 6;
    int cy = w->y + (int)W95_TITLE_H + 6;
    int cw = w->w - 12;
    int ch = w->h - (int)W95_TITLE_H - 12;

    draw_inset(cx, cy, cw, ch);

    switch (w->type) {
    case APP_TERMINAL:
        fb_fill_rect(cx + 2, cy + 2, cw - 4, ch - 4, W95_CMD_BG);
        fb_draw_string(cx + 12, cy + 12, "Microsoft Windows [Version 10.0.19045]", W10_CMD_FG, W10_CMD_BG);
        fb_draw_string(cx + 12, cy + 28, "(c) CoreOS Corporation. Todos os direitos reservados.", W10_CMD_FG, W10_CMD_BG);
        fb_draw_string(cx + 12, cy + 56, "C:\\COREOS>", W95_WHITE, W95_CMD_BG);
        fb_draw_string(cx + 120, cy + 56, "HELP", W95_CMD_FG, W95_CMD_BG);
        fb_draw_string(cx + 12, cy + 72, "DIR  CLS  VER  EXIT", W95_CMD_FG, W95_CMD_BG);
        fb_draw_string(cx + 12, cy + 100, "C:\\COREOS>", W95_WHITE, W95_CMD_BG);
        break;
    case APP_EXPLORER:
        fb_fill_rect(cx + 2, cy + 2, cw - 4, ch - 4, W95_WHITE);
        fb_draw_string(cx + 12, cy + 12, "Endereco: C:\\", W95_TEXT, W95_WHITE);
        fb_draw_string(cx + 12, cy + 36, "[DIR] WINDOWS", W95_TEXT, W95_WHITE);
        fb_draw_string(cx + 12, cy + 52, "[DIR] PROGRAM FILES", W95_TEXT, W95_WHITE);
        fb_draw_string(cx + 12, cy + 68, "[DIR] COREOS", W95_TEXT, W95_WHITE);
        fb_draw_string(cx + 12, cy + 84, "AUTOEXEC.BAT", W95_TEXT, W95_WHITE);
        fb_draw_string(cx + 12, cy + 100, "CONFIG.SYS", W95_TEXT, W95_WHITE);
        break;
    case APP_ABOUT: {
        char resbuf[24];
        const gpu_adapter_info_t *gpu = display_gpu();

        fb_fill_rect(cx + 2, cy + 2, cw - 4, ch - 4, W95_FACE);
        fb_draw_string(cx + 24, cy + 24, "CoreOS 10", W10_TEXT, W10_FACE);
        fb_draw_string(cx + 24, cy + 44, "Versao 10.0.19045", W10_TEXT, W10_FACE);
        fb_draw_string(cx + 24, cy + 68, gpu ? gpu->name : "GPU ?", W95_TEXT, W95_FACE);
        fb_draw_string(cx + 24, cy + 84, display_monitor_nome(), W95_TEXT, W95_FACE);
        display_format_resolucao(resbuf, sizeof(resbuf),
            display_monitor() ? display_monitor()->current_width : 0,
            display_monitor() ? display_monitor()->current_height : 0,
            display_monitor() ? display_monitor()->current_bpp : 0);
        fb_draw_string(cx + 24, cy + 108, "Atual: ", W95_TEXT, W95_FACE);
        fb_draw_string(cx + 88, cy + 108, resbuf, W95_TEXT, W95_FACE);
        fb_draw_string(cx + 24, cy + 132, "OpenGL + DirectX + USB", W95_TEXT, W95_FACE);
        fb_draw_string(cx + 24, cy + 156, "Copyright (C) CoreOS 2026", W95_TEXT, W95_FACE);
        draw_outset(cx + cw - 96, cy + ch - 40, 80, 28);
        fb_draw_string(cx + cw - 68, cy + ch - 32, "OK", W95_TEXT, W95_FACE);
        break;
    }
    case APP_SETTINGS:
        fb_fill_rect(cx + 2, cy + 2, cw - 4, ch - 4, W95_FACE);
        fb_draw_string(cx + 16, cy + 16, "Painel de Controle", W95_TEXT, W95_FACE);
        fb_draw_string(cx + 32, cy + 48, "Display / GPU", W95_TEXT, W95_FACE);
        draw_display_info(cx, cy);
        fb_draw_string(cx + 48, cy + 148, gl_esta_pronto() ? "OpenGL: ativo" : "OpenGL: off", W95_TEXT, W95_FACE);
        fb_draw_string(cx + 48, cy + 164, d3d_esta_pronto() ? "DirectX: ativo" : "DirectX: off", W95_TEXT, W95_FACE);
        fb_draw_string(cx + 48, cy + 180, usb_mouse_pronto() ? "Mouse: USB HID" : "Mouse: PS/2", W95_TEXT, W95_FACE);
        draw_gfx_logo(cx + cw - 100, cy + 40);
        break;
    default:
        break;
    }
}

static void draw_window(gui_window_t *w) {
    if (!w->open || w->minimized) {
        return;
    }
    draw_outset(w->x, w->y, w->w, w->h);
    draw_title_bar(w->x + 2, w->y + 2, w->w - 4, w->title, w->active);
    draw_window_content(w);
}

static void draw_icons(void) {
    int i;
    for (i = 0; i < MAX_ICONS; ++i) {
        fb_draw_string(g_icons[i].x + 20, g_icons[i].y, g_icons[i].glyph, W95_WHITE, W95_DESKTOP);
        fb_draw_string_transparent(g_icons[i].x, g_icons[i].y + 24, g_icons[i].label, W95_WHITE);
    }
}

static void draw_start_menu(const framebuffer_t *fb) {
    int mx = 8;
    int my = (int)fb->height - (int)W95_TASKBAR_H - 220;
    if (!g_start_open) {
        return;
    }
    draw_outset(mx, my, 240, 200);
    fb_fill_rect(mx + 2, my + 2, 28, 196, W95_TITLE_A);
    fb_draw_string(mx + 8, my + 80, "C", W95_WHITE, W95_TITLE_A);
    fb_draw_string(mx + 8, my + 96, "1", W95_WHITE, W95_TITLE_A);
    fb_draw_string(mx + 8, my + 112, "0", W95_WHITE, W95_TITLE_A);
    fb_draw_string(mx + 40, my + 12, "Prompt", W95_TEXT, W95_FACE);
    fb_draw_string(mx + 40, my + 36, "Meu Computador", W95_TEXT, W95_FACE);
    fb_draw_string(mx + 40, my + 60, "Painel de Controle", W95_TEXT, W95_FACE);
    fb_draw_string(mx + 40, my + 84, "Sobre", W95_TEXT, W95_FACE);
    fb_hline(mx + 36, my + 130, 200, W95_SHADOW);
    fb_draw_string(mx + 40, my + 140, "Reiniciar...", W95_TEXT, W95_FACE);
    fb_draw_string(mx + 40, my + 164, "Desligar...", W95_TEXT, W95_FACE);
}

static void draw_taskbar_clock(const framebuffer_t *fb) {
    int ty = (int)fb->height - (int)W95_TASKBAR_H;
    char timebuf[16];

    draw_inset((int)fb->width - 80, ty + 4, 72, 20);
    timebuf[0] = '1';
    timebuf[1] = '2';
    timebuf[2] = ':';
    timebuf[3] = '0' + ((g_tick / 60) % 10);
    timebuf[4] = '0' + (g_tick % 10);
    timebuf[5] = '\0';
    fb_draw_string((int)fb->width - 64, ty + 8, timebuf, W95_TEXT, W95_FACE);
}

static void draw_taskbar(const framebuffer_t *fb) {
    int ty = (int)fb->height - (int)W95_TASKBAR_H;
    int i;
    int tx = 120;

    draw_outset(0, ty, (int)fb->width, (int)W95_TASKBAR_H);
    draw_outset(6, ty + 4, 80, 20);
    fb_draw_string(16, ty + 8, "Iniciar", W10_TEXT, W10_FACE);

    for (i = 0; i < g_win_count; ++i) {
        if (!g_wins[i].open || g_wins[i].minimized) {
            continue;
        }
        draw_outset(tx, ty + 4, 140, 20);
        fb_draw_string(tx + 8, ty + 8, g_wins[i].title, W95_TEXT, W95_FACE);
        tx += 148;
    }

    draw_taskbar_clock(fb);
}

static void render_all(void) {
    const framebuffer_t *fb = fb_get();
    int i;

    mouse_hide_cursor();
    draw_wallpaper();
    draw_icons();

    for (i = 0; i < g_win_count; ++i) {
        if (!g_wins[i].active && g_wins[i].open && !g_wins[i].minimized) {
            draw_window(&g_wins[i]);
        }
    }
    for (i = 0; i < g_win_count; ++i) {
        if (g_wins[i].active && g_wins[i].open && !g_wins[i].minimized) {
            draw_window(&g_wins[i]);
        }
    }

    draw_taskbar(fb);
    draw_start_menu(fb);
    mouse_sync_cursor();
    mouse_draw_cursor();
}

static void open_app(app_type_t type) {
    switch (type) {
    case APP_TERMINAL:
        win_create(APP_TERMINAL, "Prompt", 80, 60, 480, 320);
        break;
    case APP_EXPLORER:
        win_create(APP_EXPLORER, "Meu PC", 100, 80, 460, 300);
        break;
    case APP_ABOUT:
        win_create(APP_ABOUT, "Sobre", 140, 100, 380, 260);
        break;
    case APP_SETTINGS:
        win_create(APP_SETTINGS, "Config", 120, 90, 420, 280);
        break;
    default:
        break;
    }
}

static int hit(int mx, int my, int x, int y, int w, int h) {
    return mx >= x && my >= y && mx < x + w && my < y + h;
}

static void handle_click(int mx, int my) {
    const framebuffer_t *fb = fb_get();
    int ty = (int)fb->height - (int)W95_TASKBAR_H;
    int i;

    if (hit(mx, my, 6, ty + 4, 80, 20)) {
        g_start_open = !g_start_open;
        g_dirty = 1;
        return;
    }

    if (g_start_open) {
        int smy = ty - 220;
        if (hit(mx, my, 8, smy + 12, 232, 22)) { open_app(APP_TERMINAL); g_start_open = 0; g_dirty = 1; return; }
        if (hit(mx, my, 8, smy + 36, 232, 22)) { open_app(APP_EXPLORER); g_start_open = 0; g_dirty = 1; return; }
        if (hit(mx, my, 8, smy + 60, 232, 22)) { open_app(APP_SETTINGS); g_start_open = 0; g_dirty = 1; return; }
        if (hit(mx, my, 8, smy + 84, 232, 22)) { open_app(APP_ABOUT); g_start_open = 0; g_dirty = 1; return; }
    }

    for (i = 0; i < MAX_ICONS; ++i) {
        if (hit(mx, my, g_icons[i].x, g_icons[i].y, 96, 72) && g_icons[i].type != APP_NONE) {
            open_app(g_icons[i].type);
            g_dirty = 1;
            return;
        }
    }

    for (i = g_win_count - 1; i >= 0; --i) {
        gui_window_t *w = &g_wins[i];
        if (!w->open || w->minimized) {
            continue;
        }
        if (hit(mx, my, w->x + w->w - 30, w->y + 3, 20, 16)) {
            win_close(w);
            g_dirty = 1;
            return;
        }
        if (hit(mx, my, w->x + 2, w->y + 2, w->w - 4, (int)W95_TITLE_H)) {
            int j;
            for (j = 0; j < g_win_count; ++j) {
                g_wins[j].active = 0;
            }
            w->active = 1;
            g_drag_win = i;
            g_drag_off_x = mx - w->x;
            g_drag_off_y = my - w->y;
            g_dirty = 1;
            return;
        }
    }
}

void desktop_run(void) {
    const framebuffer_t *fb;
    static uint8_t prev_buttons;

    if (!fb_init()) {
        x86_vga_init();
        x86_vga_write_at(10, 28, "ERRO: driver SVGA/VBE nao iniciou", 0x0C, 0x00);
        x86_vga_write_at(10, 30, "Verifique VM: VMSVGA + 128MB VRAM", 0x07, 0x00);
        for (;;) {
            __asm__ volatile("pause");
        }
    }

    fb = fb_get();
    drivers_marcar_mouse(mouse_init(fb->width, fb->height));
    boot_gfx_show();
    x86_keyboard_init();
    g_dirty = 1;

    for (;;) {
        key_event_t key;

        mouse_poll();
        key = x86_keyboard_poll();

        if (key == KEY_ESC) {
            g_start_open = 0;
            g_dirty = 1;
        }

        if (mouse_left_pressed() && !prev_buttons) {
            handle_click(mouse_x(), mouse_y());
        }

        if (g_drag_win >= 0 && mouse_left_pressed()) {
            g_wins[g_drag_win].x = mouse_x() - g_drag_off_x;
            g_wins[g_drag_win].y = mouse_y() - g_drag_off_y;
            if (g_wins[g_drag_win].y < 0) {
                g_wins[g_drag_win].y = 0;
            }
            g_dirty = 1;
        }
        if (!mouse_left_pressed()) {
            g_drag_win = -1;
        }

        prev_buttons = mouse_buttons();

        if (g_dirty) {
            render_all();
            g_dirty = 0;
        } else if (mouse_moved()) {
            mouse_draw_cursor();
        } else if ((++g_tick % 90) == 0) {
            mouse_hide_cursor();
            draw_taskbar_clock(fb);
            mouse_draw_cursor();
        } else {
            ++g_tick;
            __asm__ volatile("pause");
            continue;
        }

        ++g_tick;
        __asm__ volatile("pause");
    }
}
