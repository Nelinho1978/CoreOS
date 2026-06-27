#include "arch/mouse.h"
#include "arch/ports.h"
#include "coreos/usb.h"
#include "gui/fb.h"
#include "gui/win95.h"

#define MOUSE_STATUS 0x64u
#define MOUSE_DATA   0x60u
#define MOUSE_WAIT_LOOPS 20000u
#define CURSOR_W 12
#define CURSOR_H 16

static int32_t g_mx;
static int32_t g_my;
static int32_t g_prev_mx = -1;
static int32_t g_prev_my = -1;
static uint8_t g_buttons;
static int g_ready;
static int g_usb_mouse;
static uint32_t g_cursor_backup[CURSOR_W * CURSOR_H];
static int g_cursor_visible;

static const uint8_t g_arrow[CURSOR_H] = {
    0b10000000, 0b11000000, 0b11100000, 0b11110000,
    0b11111000, 0b11111100, 0b11111110, 0b11111111,
    0b11111100, 0b11011000, 0b10011000, 0b00011000,
    0b00001100, 0b00001100, 0b00000110, 0b00000000,
};

static int mouse_wait_write(void) {
    uint32_t timeout = MOUSE_WAIT_LOOPS;
    while (timeout-- > 0u && (inb(MOUSE_STATUS) & 0x02u) != 0u) {
        __asm__ volatile("pause");
    }
    return timeout > 0u;
}

static int mouse_wait_read(void) {
    uint32_t timeout = MOUSE_WAIT_LOOPS;
    while (timeout-- > 0u && (inb(MOUSE_STATUS) & 0x01u) == 0u) {
        __asm__ volatile("pause");
    }
    return timeout > 0u;
}

static int mouse_write(uint8_t value) {
    if (!mouse_wait_write()) {
        return 0;
    }
    outb(MOUSE_STATUS, 0xD4u);
    if (!mouse_wait_write()) {
        return 0;
    }
    outb(MOUSE_DATA, value);
    return 1;
}

static int mouse_read_byte(uint8_t *value) {
    if (!mouse_wait_read()) {
        return 0;
    }
    *value = inb(MOUSE_DATA);
    return 1;
}

static void mouse_flush_input(void) {
    uint32_t guard = 0;
    while (guard++ < 32u && (inb(MOUSE_STATUS) & 0x01u) != 0u) {
        (void)inb(MOUSE_DATA);
    }
}

static int ps2_mouse_init(void) {
    uint8_t status;
    uint8_t ack;

    g_ready = 0;
    g_cursor_visible = 0;

    mouse_flush_input();

    if (!mouse_wait_write()) {
        return 0;
    }
    outb(MOUSE_STATUS, 0xA8u);

    if (!mouse_wait_write()) {
        return 0;
    }
    outb(MOUSE_STATUS, 0x20u);
    if (!mouse_read_byte(&status)) {
        return 0;
    }
    status = (uint8_t)((status | 2u) & 0xFEu);

    if (!mouse_wait_write()) {
        return 0;
    }
    outb(MOUSE_STATUS, 0x60u);
    if (!mouse_wait_write()) {
        return 0;
    }
    outb(MOUSE_DATA, status);

    if (!mouse_write(0xFFu) || !mouse_read_byte(&ack) || ack != 0xFAu) {
        return 0;
    }
    if (!mouse_write(0xF6u)) {
        return 0;
    }
    if (!mouse_write(0x03u)) {
        return 0;
    }
    if (!mouse_write(0xF4u) || !mouse_read_byte(&ack) || ack != 0xFAu) {
        return 0;
    }

    mouse_flush_input();
    g_ready = 1;
    return 1;
}

int mouse_init(uint32_t screen_w, uint32_t screen_h) {
    g_mx = (int32_t)screen_w / 2;
    g_my = (int32_t)screen_h / 2;
    g_prev_mx = -1;
    g_prev_my = -1;
    g_buttons = 0;
    g_ready = 0;
    g_usb_mouse = 0;
    g_cursor_visible = 0;

    if (usb_mouse_pronto()) {
        g_usb_mouse = 1;
        g_ready = 1;
        return 1;
    }

    return ps2_mouse_init();
}

static void mouse_clamp(void) {
    const framebuffer_t *fb = fb_get();

    if (g_mx < 0) {
        g_mx = 0;
    }
    if (g_my < 0) {
        g_my = 0;
    }
    if ((uint32_t)g_mx >= fb->width) {
        g_mx = (int32_t)fb->width - 1;
    }
    if ((uint32_t)g_my >= fb->height - W95_TASKBAR_H) {
        g_my = (int32_t)fb->height - (int32_t)W95_TASKBAR_H - 1;
    }
}

static void usb_mouse_process(void) {
    int dx = usb_mouse_dx();
    int dy = usb_mouse_dy();

    g_mx += dx;
    g_my -= dy;
    g_buttons = usb_mouse_botoes();
    mouse_clamp();
}

static void mouse_process_packet(void) {
    int32_t dx;
    int32_t dy;
    uint8_t flags;

    flags = inb(MOUSE_DATA);
    dx = (int32_t)(int8_t)inb(MOUSE_DATA);
    dy = (int32_t)(int8_t)inb(MOUSE_DATA);
    (void)inb(MOUSE_DATA);

    g_mx += dx;
    g_my -= dy;
    g_buttons = flags & 0x07u;

    mouse_clamp();
}

void mouse_poll(void) {
    if (!g_ready) {
        return;
    }

    if (g_usb_mouse) {
        if (!usb_mouse_pronto()) {
            g_usb_mouse = 0;
            if (!ps2_mouse_init()) {
                g_ready = 0;
            }
            return;
        }
        usb_poll();
        usb_mouse_process();
        return;
    }

    if (!g_usb_mouse && usb_mouse_pronto()) {
        g_usb_mouse = 1;
        return;
    }

    while ((inb(MOUSE_STATUS) & 0x01u) != 0u) {
        if ((inb(MOUSE_STATUS) & 0x20u) == 0u) {
            (void)inb(MOUSE_DATA);
            continue;
        }
        mouse_process_packet();
    }
}

int mouse_moved(void) {
    return g_mx != g_prev_mx || g_my != g_prev_my;
}

int32_t mouse_x(void) { return g_mx; }
int32_t mouse_y(void) { return g_my; }
uint8_t mouse_buttons(void) { return g_buttons; }
int mouse_left_pressed(void) { return (g_buttons & 1u) != 0; }

static void cursor_save_under(void) {
    int row;
    int col;
    int idx = 0;
    for (row = 0; row < CURSOR_H; ++row) {
        for (col = 0; col < CURSOR_W; ++col) {
            g_cursor_backup[idx++] = fb_get_pixel(g_prev_mx + col, g_prev_my + row);
        }
    }
}

static void cursor_restore_under(void) {
    int row;
    int col;
    int idx = 0;
    if (g_prev_mx < 0 || g_prev_my < 0) {
        return;
    }
    for (row = 0; row < CURSOR_H; ++row) {
        for (col = 0; col < CURSOR_W; ++col) {
            fb_put_pixel(g_prev_mx + col, g_prev_my + row, g_cursor_backup[idx++]);
        }
    }
}

void mouse_hide_cursor(void) {
    if (!g_cursor_visible) {
        return;
    }
    cursor_restore_under();
    g_cursor_visible = 0;
}

void mouse_draw_cursor(void) {
    int row;
    int col;

    if (g_prev_mx != g_mx || g_prev_my != g_my) {
        if (g_cursor_visible) {
            cursor_restore_under();
        }
        g_prev_mx = g_mx;
        g_prev_my = g_my;
        cursor_save_under();
    }

    for (row = 0; row < CURSOR_H; ++row) {
        uint8_t bits = g_arrow[row];
        for (col = 0; col < 8; ++col) {
            if (bits & (0x80 >> col)) {
                fb_put_pixel(g_mx + col, g_my + row, W95_BLACK);
                if (col < 7) {
                    fb_put_pixel(g_mx + col + 1, g_my + row, W95_WHITE);
                }
            }
        }
    }
    g_cursor_visible = 1;
}

void mouse_sync_cursor(void) {
    g_prev_mx = -1;
    g_prev_my = -1;
    g_cursor_visible = 0;
}
