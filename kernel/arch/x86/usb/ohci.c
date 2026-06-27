#include "coreos/usb.h"
#include "arch/pci.h"
#include "arch/paging.h"
#include "arch/ports.h"
#include "coreos/printk.h"

#define OHCI_REV          0x00u
#define OHCI_CTRL         0x04u
#define OHCI_CMDSTS       0x08u
#define OHCI_INTR         0x0Cu
#define OHCI_INTR_EN      0x10u
#define OHCI_HCCA         0x18u
#define OHCI_ED_CTRL      0x20u
#define OHCI_ED_BULK      0x28u
#define OHCI_FM_INTERVAL  0x34u
#define OHCI_RH_DESC_A    0x48u
#define OHCI_RH_DESC_B    0x4Cu
#define OHCI_RH_STATUS    0x50u
#define OHCI_RH_PORT(n)   (0x54u + (uint32_t)(n) * 4u)

#define OHCI_CTRL_HCFS      0xC0u
#define OHCI_CTRL_USB_OPER  0x80u
#define OHCI_CTRL_USB_RESET 0x00u
#define OHCI_CMD_CLF        0x02u
#define OHCI_CMD_BLF        0x04u
#define OHCI_STS_HCH        0x01u
#define OHCI_RHPS_CCS       0x01u
#define OHCI_RHPS_PES       0x02u
#define OHCI_RHPS_PSS       0x04u
#define OHCI_RHPS_PRSC      0x10u
#define OHCI_RHPS_PRS       0x10u
#define OHCI_RHPS_PO        0x01u
#define OHCI_RHPS_PPS       0x0100u

#define TD_SETUP  (0u << 19)
#define TD_OUT    (1u << 19)
#define TD_IN     (2u << 19)
#define TD_DATA0  (0u << 24)
#define TD_DATA1  (1u << 24)
#define TD_DI     (7u << 24)
#define TD_CC     (0xFu << 28)
#define TD_DONE   (1u << 18)

typedef struct ohci_ed {
    uint32_t flags;
    uint32_t tail_p;
    uint32_t head_p;
    uint32_t next_ed;
} ohci_ed_t;

typedef struct ohci_td {
    uint32_t flags;
    uint32_t cbp;
    uint32_t next_td;
    uint32_t be;
} ohci_td_t;

typedef struct ohci_hcca {
    uint32_t intr_table[32];
    uint16_t frame;
    uint16_t pad;
    uint32_t done_head;
    uint8_t  reserved[116];
} ohci_hcca_t;

static volatile uint32_t *g_ohci;
static ohci_hcca_t __attribute__((aligned(256))) g_hcca;
static ohci_ed_t __attribute__((aligned(16))) g_ed_ctrl;
static ohci_ed_t __attribute__((aligned(16))) g_ed_intr;
static ohci_td_t __attribute__((aligned(16))) g_td_setup;
static ohci_td_t __attribute__((aligned(16))) g_td_data;
static ohci_td_t __attribute__((aligned(16))) g_td_status;
static ohci_td_t __attribute__((aligned(16))) g_td_intr;
static uint8_t __attribute__((aligned(16))) g_setup_buf[16];
static uint8_t __attribute__((aligned(16))) g_data_buf[128];
static uint8_t __attribute__((aligned(16))) g_intr_buf[8];
static int g_ohci_ok;

static uint32_t ohci_r32(uint32_t off) {
    return g_ohci[off / 4u];
}

static void ohci_w32(uint32_t off, uint32_t val) {
    g_ohci[off / 4u] = val;
}

static void ohci_delay(void) {
    uint32_t i;
    for (i = 0; i < 50000u; ++i) {
        __asm__ volatile("pause");
    }
}

static int ohci_wait_td(volatile ohci_td_t *td, uint32_t loops) {
    while (loops-- > 0u) {
        if (td->flags & TD_DONE) {
            return ((td->flags & TD_CC) >> 28) == 0u;
        }
        __asm__ volatile("pause");
    }
    return 0;
}

static void ohci_ed_init(ohci_ed_t *ed, uint8_t addr, uint8_t ep, uint16_t maxpkt, int dir_from_td) {
    ed->flags = (uint32_t)addr
        | ((uint32_t)(ep & 0xFu) << 7)
        | (dir_from_td ? (1u << 11) : 0u)
        | ((uint32_t)(maxpkt & 0x7FFu) << 16);
    ed->tail_p = 0;
    ed->head_p = 0;
    ed->next_ed = 0;
}

static int ohci_control_transfer(uint8_t addr, const uint8_t setup[8],
                                 uint8_t *data, uint16_t len, int in) {
    uint32_t pa_setup = (uint32_t)(uint32_t)setup;
    uint32_t pa_data = (uint32_t)(uint32_t)data;
    uint32_t pa_ed = (uint32_t)(uint32_t)&g_ed_ctrl;

    ohci_ed_init(&g_ed_ctrl, addr, 0, 64, 1);

    g_td_setup.flags = TD_SETUP | TD_DATA0 | TD_DI | (7u << 21);
    g_td_setup.cbp = pa_setup;
    g_td_setup.be = pa_setup + 7u;

    if (len > 0 && data) {
        g_td_setup.next_td = (uint32_t)(uint32_t)&g_td_data;

        g_td_data.flags = (in ? TD_IN : TD_OUT) | TD_DATA1 | TD_DI | (7u << 21);
        g_td_data.cbp = pa_data;
        g_td_data.next_td = (uint32_t)(uint32_t)&g_td_status;
        g_td_data.be = pa_data + (uint32_t)len - 1u;

        g_td_status.flags = (in ? TD_OUT : TD_IN) | TD_DATA1 | TD_DI | (7u << 21);
        g_td_status.cbp = 0;
        g_td_status.next_td = 0;
        g_td_status.be = 0;
    } else {
        g_td_setup.next_td = (uint32_t)(uint32_t)&g_td_data;
        g_td_data.flags = TD_IN | TD_DATA1 | TD_DI | (7u << 21);
        g_td_data.cbp = 0;
        g_td_data.next_td = 0;
        g_td_data.be = 0;
    }

    g_ed_ctrl.head_p = (uint32_t)(uint32_t)&g_td_setup;
    g_ed_ctrl.tail_p = 0;
    ohci_w32(OHCI_ED_CTRL, pa_ed);
    ohci_w32(OHCI_CMDSTS, OHCI_CMD_CLF);

    if (!ohci_wait_td(&g_td_setup, 200000u)) {
        return 0;
    }
    if (len > 0 && data) {
        if (!ohci_wait_td(&g_td_data, 200000u)) {
            return 0;
        }
        return ohci_wait_td(&g_td_status, 200000u);
    }
    return ohci_wait_td(&g_td_data, 200000u);
}

static void ohci_make_setup(uint8_t req_type, uint8_t req, uint16_t value,
                            uint16_t index, uint16_t length) {
    g_setup_buf[0] = req_type;
    g_setup_buf[1] = req;
    g_setup_buf[2] = (uint8_t)(value & 0xFFu);
    g_setup_buf[3] = (uint8_t)(value >> 8);
    g_setup_buf[4] = (uint8_t)(index & 0xFFu);
    g_setup_buf[5] = (uint8_t)(index >> 8);
    g_setup_buf[6] = (uint8_t)(length & 0xFFu);
    g_setup_buf[7] = (uint8_t)(length >> 8);
}

static int ohci_port_power(void) {
    uint32_t desc_a = ohci_r32(OHCI_RH_DESC_A);
    uint32_t ports = desc_a & 0xFFu;
    uint32_t p;

    if (ports > USB_MAX_PORTS) {
        ports = USB_MAX_PORTS;
    }

    for (p = 0; p < ports; ++p) {
        ohci_w32(OHCI_RH_PORT(p), OHCI_RHPS_PPS);
        ohci_delay();
    }
    return (int)ports;
}

static int ohci_port_reset(uint32_t port) {
    uint32_t i;
    ohci_w32(OHCI_RH_PORT(port), OHCI_RHPS_PRS);
    ohci_delay();
    for (i = 0; i < 100000u; ++i) {
        if ((ohci_r32(OHCI_RH_PORT(port)) & OHCI_RHPS_PRS) == 0u) {
            break;
        }
    }
    return (ohci_r32(OHCI_RH_PORT(port)) & OHCI_RHPS_CCS) != 0u;
}

int ohci_inicializar(const pci_device_t *dev, usb_host_t *host) {
    uint32_t bar;
    uint16_t cmd;
    uint32_t ports;
    uint32_t p;

    g_ohci_ok = 0;
    if (!dev || !host) {
        return 0;
    }

    cmd = pci_config_ler16(dev->bus, dev->device, dev->function, 4);
    pci_config_escrever32(dev->bus, dev->device, dev->function, 4, (uint32_t)cmd | 0x06u);

    bar = pci_ler_bar(dev->bus, dev->device, dev->function, 0);
    if (bar == 0) {
        return 0;
    }

    paging_map_mmio(bar, 4096u);
    g_ohci = (volatile uint32_t *)(uint32_t)bar;

    ohci_w32(OHCI_CTRL, OHCI_CTRL_USB_RESET);
    ohci_delay();
    ohci_w32(OHCI_INTR_EN, 0);
    ohci_w32(OHCI_HCCA, (uint32_t)(uint32_t)&g_hcca);
    ohci_w32(OHCI_ED_CTRL, 0);
    ohci_w32(OHCI_ED_BULK, 0);
    ohci_w32(OHCI_FM_INTERVAL, 0x2EDF);
    ohci_w32(OHCI_CTRL, OHCI_CTRL_USB_OPER);

    ports = (uint32_t)ohci_port_power();
    host->port_count = (uint8_t)ports;
    host->mmio_base = bar;
    host->type = USB_HCI_OHCI;
    host->bus = dev->bus;
    host->device = dev->device;
    host->function = dev->function;
    host->active = 1;

    for (p = 0; p < ports; ++p) {
        host->ports[p].state = USB_PORT_EMPTY;
        host->ports[p].powered = 1;
        if ((ohci_r32(OHCI_RH_PORT(p)) & OHCI_RHPS_CCS) != 0u) {
            host->ports[p].state = USB_PORT_CONNECTED;
        }
    }

    g_ohci_ok = 1;
    kputs("[USB] OHCI host OK — portas=");
    kprintf("%u", ports);
    kputs("\n");
    return 1;
}

int ohci_esta_pronto(void) {
    return g_ohci_ok;
}

int ohci_reset_porta(uint32_t port) {
    if (!g_ohci_ok) {
        return 0;
    }
    return ohci_port_reset(port);
}

int ohci_get_descriptor(uint8_t addr, uint8_t type, uint8_t index,
                        uint8_t *out, uint16_t maxlen) {
    ohci_make_setup(0x80u, 0x06u, (uint16_t)((type << 8) | index), 0, maxlen);
    return ohci_control_transfer(addr, g_setup_buf, out, maxlen, 1);
}

int ohci_set_address(uint8_t new_addr) {
    ohci_make_setup(0x00u, 0x05u, new_addr, 0, 0);
    return ohci_control_transfer(0, g_setup_buf, NULL, 0, 0);
}

int ohci_set_configuration(uint8_t addr, uint8_t config) {
    ohci_make_setup(0x00u, 0x09u, config, 0, 0);
    return ohci_control_transfer(addr, g_setup_buf, NULL, 0, 0);
}

int ohci_intr_setup(uint8_t addr, uint8_t ep, uint16_t maxpkt) {
    uint32_t pa_ed = (uint32_t)(uint32_t)&g_ed_intr;
    ohci_ed_init(&g_ed_intr, addr, ep & 0x0Fu, maxpkt, 0);
    g_td_intr.flags = TD_IN | TD_DATA0 | TD_DI | (7u << 21);
    g_td_intr.cbp = (uint32_t)(uint32_t)g_intr_buf;
    g_td_intr.next_td = 0;
    g_td_intr.be = (uint32_t)(uint32_t)(g_intr_buf + maxpkt - 1u);
    g_ed_intr.head_p = (uint32_t)(uint32_t)&g_td_intr;
    g_ed_intr.tail_p = 0;
    ohci_w32(OHCI_ED_CTRL, pa_ed);
    ohci_w32(OHCI_CMDSTS, OHCI_CMD_CLF);
    return 1;
}

int ohci_intr_poll(uint8_t *report, uint8_t maxlen) {
    uint32_t i;
    if (!g_ohci_ok) {
        return 0;
    }
    if (!(g_td_intr.flags & TD_DONE)) {
        return 0;
    }
    if (((g_td_intr.flags & TD_CC) >> 28) != 0u) {
        ohci_intr_setup(g_ed_intr.flags & 0x7Fu, (uint8_t)((g_ed_intr.flags >> 7) & 0xFu), 8);
        return 0;
    }
    for (i = 0; i < maxlen && i < 8u; ++i) {
        report[i] = g_intr_buf[i];
    }
    g_td_intr.flags &= ~TD_DONE;
    ohci_w32(OHCI_CMDSTS, OHCI_CMD_CLF);
    ohci_wait_td(&g_td_intr, 200000u);
    return (int)maxlen;
}

uint8_t *ohci_data_buffer(void) {
    return g_data_buf;
}
