#include "arch/paging.h"
#include "arch/ports.h"

static uint32_t __attribute__((aligned(4096))) g_pdir[1024];
static uint32_t __attribute__((aligned(4096))) g_ptbl_low[1024];
static uint32_t __attribute__((aligned(4096))) g_ptbl_fb[1024];
static uint32_t __attribute__((aligned(4096))) g_ptbl_mmio[2][1024];
static int g_paging_on;
static int g_mmio_count;
static uint32_t g_mmio_pdi[2];

static int paging_mmio_slot(uint32_t pdi) {
    int i;
    for (i = 0; i < g_mmio_count; ++i) {
        if (g_mmio_pdi[i] == pdi) {
            return i;
        }
    }
    if (g_mmio_count < 2) {
        g_mmio_pdi[g_mmio_count] = pdi;
        return g_mmio_count++;
    }
    return -1;
}

static void paging_map_range(uint32_t phys, uint32_t bytes, uint32_t *ptbl, int merge_low) {
    uint32_t i;
    uint32_t pd_index;
    uint32_t pages;
    uint32_t page_index;

    if (phys == 0 || bytes == 0) {
        return;
    }

    pages = (bytes + 4095u) / 4096u;
    if (pages > 1024u) {
        pages = 1024u;
    }

    pd_index = phys >> 22;

    if (merge_low && pd_index == 0) {
        for (i = 0; i < pages; ++i) {
            page_index = (phys + i * 4096u) >> 12;
            if (page_index < 1024u) {
                g_ptbl_low[page_index] = (phys + i * 4096u) | 0x03u;
            }
        }
        return;
    }

    if (pd_index < 1024u) {
        for (i = 0; i < pages; ++i) {
            ptbl[i] = (phys + i * 4096u) | 0x03u;
        }
        g_pdir[pd_index] = ((uint32_t)ptbl) | 0x03u;
    }
}

void paging_map_mmio(uint32_t phys, uint32_t bytes) {
    int slot;
    uint32_t pdi;

    if (!g_paging_on) {
        return;
    }

    pdi = phys >> 22;
    slot = paging_mmio_slot(pdi);
    if (slot < 0) {
        return;
    }

    paging_map_range(phys, bytes, g_ptbl_mmio[slot], 0);
    __asm__ volatile("mov %0, %%cr3" ::"r"(g_pdir) : "memory");
}

void paging_init(uint32_t fb_phys, uint32_t fb_bytes) {
    uint32_t i;

    for (i = 0; i < 1024; ++i) {
        g_ptbl_low[i] = (i * 4096u) | 0x03u;
    }
    g_pdir[0] = ((uint32_t)g_ptbl_low) | 0x03u;

    for (i = 1; i < 1024; ++i) {
        g_pdir[i] = 0;
    }

    if (fb_phys != 0 && fb_bytes != 0) {
        if ((fb_phys >> 22) == 0) {
            paging_map_range(fb_phys, fb_bytes, g_ptbl_low, 1);
        } else {
            paging_map_range(fb_phys, fb_bytes, g_ptbl_fb, 0);
        }
    }

    __asm__ volatile("mov %0, %%cr3" ::"r"(g_pdir) : "memory");

    {
        uint32_t cr0;
        __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
        cr0 |= 0x80000000u;
        __asm__ volatile("mov %0, %%cr0" ::"r"(cr0) : "memory");
    }

    g_paging_on = 1;
}
