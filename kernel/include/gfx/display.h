#ifndef COREOS_GFX_DISPLAY_H
#define COREOS_GFX_DISPLAY_H

#include "coreos/types.h"

#define DISPLAY_MAX_MODES 12u
#define DISPLAY_NAME_LEN  56u
#define DISPLAY_VENDOR_LEN 24u

typedef struct display_mode_info {
    uint16_t mode_id;
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint8_t linear;
} display_mode_info_t;

typedef struct gpu_adapter_info {
    char name[DISPLAY_NAME_LEN];
    char vendor[DISPLAY_VENDOR_LEN];
    char chip_class[20];
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint8_t subclass;
    uint32_t vram_kb;
    uint16_t bochs_vbe_id;
    int active;
} gpu_adapter_info_t;

typedef struct monitor_info {
    char name[32];
    uint32_t max_width;
    uint32_t max_height;
    uint32_t preferred_width;
    uint32_t preferred_height;
    uint32_t current_width;
    uint32_t current_height;
    uint8_t current_bpp;
    uint8_t mode_count;
    uint16_t vbe_version;
    uint16_t selected_mode_id;
} monitor_info_t;

typedef struct display_model {
    gpu_adapter_info_t gpu;
    monitor_info_t monitor;
    display_mode_info_t modes[DISPLAY_MAX_MODES];
    int ready;
} display_model_t;

int display_detect_init(void);
const display_model_t *display_modelo(void);
const gpu_adapter_info_t *display_gpu(void);
const monitor_info_t *display_monitor(void);
const char *display_gpu_nome(void);
const char *display_monitor_nome(void);
uint32_t display_modo_count(void);
const display_mode_info_t *display_modo(uint32_t index);
void display_resolucao_atual(uint32_t *w, uint32_t *h, uint8_t *bpp);
void display_format_resolucao(char *buf, size_t len, uint32_t w, uint32_t h, uint8_t bpp);
int display_modo_contem(uint32_t width, uint32_t height, uint8_t bpp);

#endif
