#ifndef COREOS_USB_H
#define COREOS_USB_H

#include "coreos/types.h"

#define USB_MAX_HOSTS   2u
#define USB_MAX_PORTS   8u
#define USB_MAX_DEVICES 4u

typedef enum {
    USB_SPEED_LOW = 0,
    USB_SPEED_FULL = 1,
    USB_SPEED_HIGH = 2,
} usb_speed_t;

typedef enum {
    USB_HCI_NONE = 0,
    USB_HCI_UHCI = 1,
    USB_HCI_OHCI = 2,
} usb_hci_type_t;

typedef enum {
    USB_PORT_EMPTY = 0,
    USB_PORT_CONNECTED = 1,
    USB_PORT_ENABLED = 2,
} usb_port_state_t;

typedef struct usb_port {
    usb_port_state_t state;
    usb_speed_t speed;
    int powered;
} usb_port_t;

typedef struct usb_host {
    usb_hci_type_t type;
    char name[24];
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint32_t mmio_base;
    usb_port_t ports[USB_MAX_PORTS];
    uint8_t port_count;
    int active;
} usb_host_t;

typedef struct usb_device {
    int present;
    uint8_t address;
    uint8_t interface;
    uint8_t ep_in;
    uint8_t ep_maxpkt;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t protocol;
    usb_speed_t speed;
} usb_device_t;

typedef struct usb_model {
    usb_host_t hosts[USB_MAX_HOSTS];
    uint8_t host_count;
    usb_device_t devices[USB_MAX_DEVICES];
    uint8_t device_count;
    int hid_mouse_ready;
    int stack_ready;
} usb_model_t;

int usb_inicializar(void);
const usb_model_t *usb_modelo(void);
void usb_poll(void);
int usb_mouse_pronto(void);
int usb_mouse_dx(void);
int usb_mouse_dy(void);
uint8_t usb_mouse_botoes(void);

#endif
