#include "coreos/usb.h"
#include "arch/pci.h"
#include "arch/paging.h"
#include "arch/ports.h"
#include "coreos/printk.h"

#define UHCI_USBCMD   0x00u
#define UHCI_USBSTS   0x02u
#define UHCI_USBINTR  0x04u
#define UHCI_FRNUM    0x06u
#define UHCI_FLBASE   0x08u
#define UHCI_SOFMOD   0x0Cu
#define UHCI_PORTSC1  0x10u

#define UHCI_CMD_RUN  0x01u
#define UHCI_CMD_GRESET 0x02u
#define UHCI_STS_HCH  0x20u
#define UHCI_PORT_CCS 0x01u
#define UHCI_PORT_EN  0x02u
#define UHCI_PORT_RESET 0x10u

#define UHCI_PID_SETUP 0x2Du
#define UHCI_PID_IN    0x69u
#define UHCI_PID_OUT   0xE1u

typedef struct uhci_qh {
    uint32_t link;
    uint32_t elink;
    uint32_t status;
    uint32_t token;
    uint32_t buffer;
} __attribute__((packed)) uhci_qh_t;

typedef struct uhci_td {
    uint32_t link;
    uint32_t status;
    uint32_t token;
    uint32_t buffer;
} __attribute__((packed)) uhci_td_t;

static uint16_t g_uhci_base;
static uint32_t __attribute__((aligned(4096))) g_frame_list[1024];
static uhci_qh_t __attribute__((aligned(32))) g_qh_ctrl;
static uhci_td_t __attribute__((aligned(32))) g_td_setup;
static uhci_td_t __attribute__((aligned(32))) g_td_data;
static uhci_td_t __attribute__((aligned(32))) g_td_status;
static uhci_td_t __attribute__((aligned(32))) g_td_intr;
static uint8_t __attribute__((aligned(64))) g_setup[16];
static uint8_t __attribute__((aligned(64))) g_data[128];
static uint8_t __attribute__((aligned(64))) g_intr[8];
static int g_uhci_ok;

static uint16_t uhci_inw(uint16_t reg) {
    return inw((uint16_t)(g_uhci_base + reg));
}

static void uhci_outw(uint16_t reg, uint16_t val) {
    outw((uint16_t)(g_uhci_base + reg), val);
}

static void uhci_delay(void) {
    uint32_t i;
    for (i = 0; i < 50000u; ++i) {
        __asm__ volatile("pause");
    }
}

static int uhci_wait_td(volatile uhci_td_t *td, uint32_t loops) {
    while (loops-- > 0u) {
        if ((td->status & (1u << 23)) == 0u) {
            return (td->status & 0x7Fu) == 0u;
        }
        __asm__ volatile("pause");
    }
    return 0;
}

static void uhci_td_init(uhci_td_t *td, uint32_t pid, uint8_t addr, uint8_t ep,
                         uint8_t *buf, uint16_t len, int spd) {
    uint32_t token = (uint32_t)len << 21;
    token |= (uint32_t)(pid & 0xFFu) << 8;
    token |= (uint32_t)((ep & 0xFu) << 15);
    token |= (uint32_t)(addr & 0x7Fu);
    token |= spd ? (1u << 19) : 0u;
    td->link = 1u;
    td->status = (1u << 23) | (1u << 15) | (0x7Fu << 16);
    td->token = token;
    td->buffer = (uint32_t)(uint32_t)buf;
}

static void uhci_make_setup(uint8_t req_type, uint8_t req, uint16_t value,
                            uint16_t index, uint16_t length) {
    g_setup[0] = req_type;
    g_setup[1] = req;
    g_setup[2] = (uint8_t)(value & 0xFFu);
    g_setup[3] = (uint8_t)(value >> 8);
    g_setup[4] = (uint8_t)(index & 0xFFu);
    g_setup[5] = (uint8_t)(index >> 8);
    g_setup[6] = (uint8_t)(length & 0xFFu);
    g_setup[7] = (uint8_t)(length >> 8);
}

static int uhci_control_transfer(uint8_t addr, const uint8_t setup[8],
                                 uint8_t *data, uint16_t len, int in) {
    g_qh_ctrl.link = (uint32_t)(uint32_t)&g_qh_ctrl | 2u;
    g_qh_ctrl.elink = 1u;
    g_qh_ctrl.status = 0;
    g_qh_ctrl.token = 0;
    g_qh_ctrl.buffer = 0;

    uhci_td_init(&g_td_setup, UHCI_PID_SETUP, addr, 0, (uint8_t *)(uint32_t)setup, 8, 0);
    g_td_setup.link = (uint32_t)(uint32_t)&g_td_data;

    if (len > 0 && data) {
        uhci_td_init(&g_td_data, in ? UHCI_PID_IN : UHCI_PID_OUT, addr, 0, data, len, 0);
        g_td_data.link = (uint32_t)(uint32_t)&g_td_status;
        uhci_td_init(&g_td_status, in ? UHCI_PID_OUT : UHCI_PID_IN, addr, 0, NULL, 0, 0);
    } else {
        uhci_td_init(&g_td_data, UHCI_PID_IN, addr, 0, NULL, 0, 0);
    }
    g_td_status.link = 1u;

    g_qh_ctrl.link = (uint32_t)(uint32_t)&g_td_setup;
    g_frame_list[0] = (uint32_t)(uint32_t)&g_qh_ctrl;

    if (!uhci_wait_td(&g_td_setup, 200000u)) {
        return 0;
    }
    if (len > 0 && data && !uhci_wait_td(&g_td_data, 200000u)) {
        return 0;
    }
    if (!uhci_wait_td(&g_td_status, 200000u)) {
        if (len == 0 && !uhci_wait_td(&g_td_data, 200000u)) {
            return 0;
        }
        if (len > 0) {
            return 0;
        }
    }
    return 1;
}

int uhci_inicializar(const pci_device_t *dev, usb_host_t *host) {
    uint16_t cmd;
    uint32_t i;

    g_uhci_ok = 0;
    if (!dev || !host) {
        return 0;
    }

    cmd = pci_config_ler16(dev->bus, dev->device, dev->function, 4);
    pci_config_escrever32(dev->bus, dev->device, dev->function, 4, (uint32_t)cmd | 0x07u);

    g_uhci_base = pci_ler_bar_io(dev->bus, dev->device, dev->function, 4);
    if (g_uhci_base == 0) {
        g_uhci_base = pci_ler_bar_io(dev->bus, dev->device, dev->function, 0);
    }
    if (g_uhci_base == 0) {
        return 0;
    }

    uhci_outw(UHCI_USBCMD, 0);
    uhci_delay();
    uhci_outw(UHCI_USBCMD, UHCI_CMD_GRESET);
    uhci_delay();
    uhci_outw(UHCI_USBCMD, 0);
    uhci_delay();

    for (i = 0; i < 1024u; ++i) {
        g_frame_list[i] = 1u;
    }

    uhci_outw(UHCI_FLBASE, (uint16_t)((uint32_t)(uint32_t)g_frame_list & 0xFFFFu));
    uhci_outw(UHCI_FLBASE + 2u, (uint16_t)(((uint32_t)(uint32_t)g_frame_list >> 16) & 0xFFFFu));
    uhci_outw(UHCI_SOFMOD, 64);
    uhci_outw(UHCI_USBCMD, UHCI_CMD_RUN);

    host->type = USB_HCI_UHCI;
    host->bus = dev->bus;
    host->device = dev->device;
    host->function = dev->function;
    host->mmio_base = g_uhci_base;
    host->port_count = 2;
    host->ports[0].powered = 1;
    host->ports[1].powered = 1;
    host->ports[0].state = (uhci_inw(UHCI_PORTSC1) & UHCI_PORT_CCS) ? USB_PORT_CONNECTED : USB_PORT_EMPTY;
    host->ports[1].state = (uhci_inw((uint16_t)(UHCI_PORTSC1 + 2u)) & UHCI_PORT_CCS) ? USB_PORT_CONNECTED : USB_PORT_EMPTY;
    host->active = 1;
    g_uhci_ok = 1;

    kputs("[USB] UHCI host OK — base=");
    kprint_hex(g_uhci_base);
    kputs("\n");
    return 1;
}

int uhci_esta_pronto(void) {
    return g_uhci_ok;
}

int uhci_reset_porta(uint32_t port) {
    uint16_t reg = (uint16_t)(UHCI_PORTSC1 + port * 2u);
    if (!g_uhci_ok) {
        return 0;
    }
    uhci_outw(reg, uhci_inw(reg) | UHCI_PORT_RESET);
    uhci_delay();
    uhci_outw(reg, uhci_inw(reg) & (uint16_t)~UHCI_PORT_RESET);
    uhci_outw(reg, uhci_inw(reg) | UHCI_PORT_EN);
    return (uhci_inw(reg) & UHCI_PORT_CCS) != 0;
}

int uhci_get_descriptor(uint8_t addr, uint8_t type, uint8_t index,
                        uint8_t *out, uint16_t maxlen) {
    uhci_make_setup(0x80u, 0x06u, (uint16_t)((type << 8) | index), 0, maxlen);
    return uhci_control_transfer(addr, g_setup, out, maxlen, 1);
}

int uhci_set_address(uint8_t new_addr) {
    uhci_make_setup(0x00u, 0x05u, new_addr, 0, 0);
    return uhci_control_transfer(0, g_setup, NULL, 0, 0);
}

int uhci_set_configuration(uint8_t addr, uint8_t config) {
    uhci_make_setup(0x00u, 0x09u, config, 0, 0);
    return uhci_control_transfer(addr, g_setup, NULL, 0, 0);
}

int uhci_intr_setup(uint8_t addr, uint8_t ep, uint16_t maxpkt) {
    (void)maxpkt;
    uhci_td_init(&g_td_intr, UHCI_PID_IN, addr, ep & 0x0Fu, g_intr, 4, 0);
    g_td_intr.link = 1u;
    g_qh_ctrl.link = (uint32_t)(uint32_t)&g_td_intr;
    g_frame_list[0] = (uint32_t)(uint32_t)&g_qh_ctrl;
    return 1;
}

int uhci_intr_poll(uint8_t *report, uint8_t maxlen) {
    uint32_t i;
    if (!g_uhci_ok || (g_td_intr.status & (1u << 23)) != 0u) {
        return 0;
    }
    for (i = 0; i < maxlen && i < 4u; ++i) {
        report[i] = g_intr[i];
    }
    g_td_intr.status = (1u << 23) | (1u << 15) | (0x7Fu << 16);
    uhci_intr_setup(g_td_intr.token & 0x7Fu, (uint8_t)((g_td_intr.token >> 15) & 0xFu), 4);
    return (int)maxlen;
}

uint8_t *uhci_data_buffer(void) {
    return g_data;
}
