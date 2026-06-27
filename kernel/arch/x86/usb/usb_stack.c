#include "coreos/usb.h"
#include "arch/ohci.h"
#include "arch/uhci.h"
#include "arch/pci.h"
#include "coreos/printk.h"

static usb_model_t g_usb;
static usb_hci_type_t g_hci = USB_HCI_NONE;
static int g_mx;
static int g_my;
static int g_dx;
static int g_dy;
static uint8_t g_buttons;
static uint8_t g_mouse_addr;
static uint8_t g_mouse_ep;

static int usb_parse_hid_mouse(const uint8_t *cfg, uint32_t len, uint8_t *ep_out) {
    uint32_t i = 0;

    while (i + 8 < len) {
        uint8_t desc_type = cfg[i + 1];
        uint8_t desc_len = cfg[i];

        if (desc_len == 0) {
            break;
        }

        if (desc_type == 0x04u && desc_len >= 9u) {
            uint8_t cls = cfg[i + 5];
            uint8_t sub = cfg[i + 6];
            uint8_t proto = cfg[i + 7];
            if (cls == 0x03u && sub == 0x01u && proto == 0x02u) {
                /* interface HID boot mouse */
            }
        }

        if (desc_type == 0x05u && desc_len >= 7u) {
            uint8_t ep = cfg[i + 2];
            uint8_t attr = cfg[i + 3];
            if ((ep & 0x80u) != 0u && (attr & 0x03u) == 0x03u) {
                *ep_out = ep;
                return 1;
            }
        }

        i += desc_len;
    }
    return 0;
}

static int usb_enumerate_mouse(void) {
    uint8_t *buf;
    uint16_t cfg_len;
    uint8_t ep = 0;

    if (g_hci == USB_HCI_OHCI) {
        if (!ohci_reset_porta(0)) {
            return 0;
        }
        buf = ohci_data_buffer();
        if (!ohci_get_descriptor(0, 0x01u, 0, buf, 8)) {
            return 0;
        }
        if (!ohci_set_address(1)) {
            return 0;
        }
        g_mouse_addr = 1;
        if (!ohci_get_descriptor(1, 0x01u, 0, buf, 18)) {
            return 0;
        }
        if (!ohci_get_descriptor(1, 0x02u, 0, buf, 9)) {
            return 0;
        }
        cfg_len = (uint16_t)buf[2] | ((uint16_t)buf[3] << 8);
        if (cfg_len > 120u) {
            cfg_len = 120u;
        }
        if (!ohci_get_descriptor(1, 0x02u, 0, buf, cfg_len)) {
            return 0;
        }
        if (!usb_parse_hid_mouse(buf, cfg_len, &ep)) {
            ep = 0x81u;
        }
        if (!ohci_set_configuration(1, 1)) {
            return 0;
        }
        g_mouse_ep = ep;
        ohci_intr_setup(g_mouse_addr, g_mouse_ep, 8);
        return 1;
    }

    if (g_hci == USB_HCI_UHCI) {
        if (!uhci_reset_porta(0)) {
            return 0;
        }
        buf = uhci_data_buffer();
        if (!uhci_get_descriptor(0, 0x01u, 0, buf, 8)) {
            return 0;
        }
        if (!uhci_set_address(1)) {
            return 0;
        }
        g_mouse_addr = 1;
        if (!uhci_get_descriptor(1, 0x01u, 0, buf, 18)) {
            return 0;
        }
        if (!uhci_get_descriptor(1, 0x02u, 0, buf, 9)) {
            return 0;
        }
        cfg_len = (uint16_t)buf[2] | ((uint16_t)buf[3] << 8);
        if (cfg_len > 120u) {
            cfg_len = 120u;
        }
        if (!uhci_get_descriptor(1, 0x02u, 0, buf, cfg_len)) {
            return 0;
        }
        if (!usb_parse_hid_mouse(buf, cfg_len, &ep)) {
            ep = 0x81u;
        }
        if (!uhci_set_configuration(1, 1)) {
            return 0;
        }
        g_mouse_ep = ep;
        uhci_intr_setup(g_mouse_addr, g_mouse_ep, 4);
        return 1;
    }

    return 0;
}

int usb_inicializar(void) {
    pci_device_t dev;
    usb_host_t *host;

    if (g_usb.stack_ready) {
        return 1;
    }

    g_usb.host_count = 0;
    g_usb.device_count = 0;
    g_usb.hid_mouse_ready = 0;
    g_hci = USB_HCI_NONE;
    g_dx = 0;
    g_dy = 0;
    g_buttons = 0;

    kputs("[USB] Inicializando modelo de conexoes USB...\n");

    if (pci_encontrar_usb(0x10u, &dev)) {
        host = &g_usb.hosts[g_usb.host_count];
        host->name[0] = 'O';
        host->name[1] = 'H';
        host->name[2] = 'C';
        host->name[3] = 'I';
        host->name[4] = '\0';
        if (ohci_inicializar(&dev, host)) {
            g_hci = USB_HCI_OHCI;
            ++g_usb.host_count;
        }
    }

    if (g_hci == USB_HCI_NONE && pci_encontrar_usb(0x00u, &dev)) {
        host = &g_usb.hosts[g_usb.host_count];
        host->name[0] = 'U';
        host->name[1] = 'H';
        host->name[2] = 'C';
        host->name[3] = 'I';
        host->name[4] = '\0';
        if (uhci_inicializar(&dev, host)) {
            g_hci = USB_HCI_UHCI;
            ++g_usb.host_count;
        }
    }

    if (g_hci == USB_HCI_NONE) {
        kputs("[USB] Nenhum host USB (OHCI/UHCI) encontrado\n");
        return 0;
    }

    if (usb_enumerate_mouse()) {
        g_usb.hid_mouse_ready = 1;
        g_usb.devices[0].present = 1;
        g_usb.devices[0].address = g_mouse_addr;
        g_usb.devices[0].ep_in = g_mouse_ep;
        g_usb.devices[0].class_code = 0x03u;
        g_usb.devices[0].subclass = 0x01u;
        g_usb.devices[0].protocol = 0x02u;
        g_usb.device_count = 1;
        kputs("[USB] Mouse HID boot conectado (addr=1)\n");
    } else {
        kputs("[USB] Host OK — aguardando mouse USB na porta\n");
    }

    g_usb.stack_ready = 1;
    return 1;
}

const usb_model_t *usb_modelo(void) {
    return &g_usb;
}

void usb_poll(void) {
    uint8_t report[4];
    int n;

    if (!g_usb.stack_ready || !g_usb.hid_mouse_ready) {
        if (g_usb.stack_ready && !g_usb.hid_mouse_ready) {
            if (usb_enumerate_mouse()) {
                g_usb.hid_mouse_ready = 1;
                kputs("[USB] Mouse HID conectado\n");
            }
        }
        return;
    }

    if (g_hci == USB_HCI_OHCI) {
        n = ohci_intr_poll(report, 3);
    } else {
        n = uhci_intr_poll(report, 3);
    }

    if (n < 3) {
        return;
    }

    g_buttons = report[0] & 0x07u;
    g_dx += (int8_t)report[1];
    g_dy += (int8_t)report[2];
}

int usb_mouse_pronto(void) {
    return g_usb.hid_mouse_ready;
}

int usb_mouse_dx(void) {
    int v = g_dx;
    g_dx = 0;
    return v;
}

int usb_mouse_dy(void) {
    int v = g_dy;
    g_dy = 0;
    return v;
}

uint8_t usb_mouse_botoes(void) {
    return g_buttons;
}

void usb_mouse_sync_pos(int x, int y) {
    g_mx = x;
    g_my = y;
}

int usb_mouse_x(void) {
    return g_mx;
}

int usb_mouse_y(void) {
    return g_my;
}

void usb_mouse_move(int dx, int dy) {
    g_mx += dx;
    g_my += dy;
}
