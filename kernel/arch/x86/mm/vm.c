#include "arch/vm.h"
#include "arch/paging.h"
#include "coreos/printk.h"
#include "coreos/memory.h"
#include "ntos/mm.h"

/* ---- External: page table operations (from paging.c) ---- */
extern uint32_t *paging_get_page_directory(void);

/* ---- Physical Page Bitmap ---- */
/* Simple bitmap for tracking physical pages (4KB each) */
#define BITS_PER_PAGE_BITMAP ((VM_MAX_PAGES + 31) / 32)
static uint32_t g_page_bitmap[BITS_PER_PAGE_BITMAP];
static uint32_t g_total_pages;
static uint32_t g_used_pages;

/* ---- VM Regions (for tracking virtual memory allocations) ---- */
#define VM_MAX_REGIONS 64
static VM_REGION g_regions[VM_MAX_REGIONS];
static int g_vm_initialized;

/* ---- Kernel heap state ---- */
#define HEAP_START   0xE0000000  /* Kernel heap at 3.5GB */
#define HEAP_SIZE    0x10000000  /* 256 MB heap */
static uint32_t g_heap_brk = HEAP_START;

/* ---- Page table entry manipulation ---- */
#define PD_INDEX(vaddr)   ((vaddr) >> 22)
#define PT_INDEX(vaddr)   (((vaddr) >> 12) & 0x3FF)

/* ---- Bitmap helpers ---- */
static void bitmap_set(uint32_t *bitmap, uint32_t bit) {
    bitmap[bit / 32] |= (1u << (bit % 32));
}

static void bitmap_clear(uint32_t *bitmap, uint32_t bit) {
    bitmap[bit / 32] &= ~(1u << (bit % 32));
}

static int bitmap_test(uint32_t *bitmap, uint32_t bit) {
    return (bitmap[bit / 32] >> (bit % 32)) & 1;
}

/* ================================================================
 *  Initialization
 * ================================================================ */

void vm_init(void) {
    const memory_map_t *map;
    uint32_t i;

    if (g_vm_initialized) return;

    kputs("[VM] Inicializando gerenciador de memoria virtual...\n");

    /* Clear bitmap */
    for (i = 0; i < BITS_PER_PAGE_BITMAP; ++i)
        g_page_bitmap[i] = 0;

    g_total_pages = 0;
    g_used_pages = 0;

    /* Mark all pages in memory map as used initially */
    map = MmGetPhysicalMemoryMap();
    if (map) {
        for (i = 0; i < map->count; ++i) {
            uint32_t base = (uint32_t)(map->regions[i].base);
            uint32_t len  = (uint32_t)(map->regions[i].length);
            uint32_t end  = base + len;
            uint32_t page;

            for (page = base / 4096; page < (end + 4095) / 4096 && page < VM_MAX_PAGES; ++page) {
                if (map->regions[i].type == 1) { /* MULTIBOOT_MEMORY_AVAILABLE */
                    bitmap_clear(g_page_bitmap, page);
                } else {
                    bitmap_set(g_page_bitmap, page);
                }
            }

            if (end / 4096 > g_total_pages)
                g_total_pages = end / 4096;
        }
    }

    if (g_total_pages == 0) {
        /* Fallback: assume 32MB RAM = 8192 pages */
        g_total_pages = 8192;
    }

    /* Mark first 1MB as used (BIOS, VGA, boot code) */
    for (i = 0; i < 256; ++i)
        bitmap_set(g_page_bitmap, i);

    /* Mark kernel pages as used (0x10000 to wherever kernel ends) */
    /* We'll mark a generous 2MB for kernel */
    for (i = 0x10000 / 4096; i < (0x10000 + 0x200000) / 4096 && i < VM_MAX_PAGES; ++i)
        bitmap_set(g_page_bitmap, i);

    /* Mark framebuffer pages as used */
    {
        void *fb_addr = (void *)0xFD000000;  /* Typical framebuffer area */
        uint32_t fb_pages = 4 * 1024 * 1024 / 4096;  /* 4MB */
        uint32_t fb_page_start = (uint32_t)fb_addr / 4096;
        for (i = fb_page_start; i < fb_page_start + fb_pages && i < VM_MAX_PAGES; ++i)
            bitmap_set(g_page_bitmap, i);
    }

    /* Clear VM regions */
    for (i = 0; i < VM_MAX_REGIONS; ++i)
        g_regions[i].used = 0;

    g_vm_initialized = 1;
    kprintf("[VM] %u paginas fisicas (%u KB RAM)\n", g_total_pages, g_total_pages * 4);
    kprintf("[VM] Paginas usadas: %u\n", g_used_pages);
}

/* ================================================================
 *  Physical Page Management
 * ================================================================ */

void vm_mark_page_used(uint32_t phys_addr) {
    uint32_t page = phys_addr / 4096;
    if (page < VM_MAX_PAGES && !bitmap_test(g_page_bitmap, page)) {
        bitmap_set(g_page_bitmap, page);
        ++g_used_pages;
    }
}

void vm_mark_range_used(uint32_t base, uint32_t size) {
    uint32_t end = base + size;
    uint32_t page;
    for (page = base / 4096; page <= end / 4096 && page < VM_MAX_PAGES; ++page)
        vm_mark_page_used(page * 4096);
}

uint32_t vm_alloc_page(void) {
    uint32_t i;
    for (i = 0; i < g_total_pages && i < VM_MAX_PAGES; ++i) {
        if (!bitmap_test(g_page_bitmap, i)) {
            bitmap_set(g_page_bitmap, i);
            ++g_used_pages;
            return i * 4096;
        }
    }
    kputs("[VM] ERRO: sem paginas fisicas disponiveis!\n");
    return 0;
}

void vm_free_page(uint32_t phys_addr) {
    uint32_t page = phys_addr / 4096;
    if (page < VM_MAX_PAGES && bitmap_test(g_page_bitmap, page)) {
        bitmap_clear(g_page_bitmap, page);
        if (g_used_pages > 0) --g_used_pages;
    }
}

/* ================================================================
 *  Page Table Operations
 * ================================================================ */

/* Create page table entry from physical address and flags */
static uint32_t make_pte(uint32_t phys_addr, uint32_t flags) {
    return (phys_addr & 0xFFFFF000) | (flags & 0xFFF);
}

/* Get or create page table for a given PD index */
static uint32_t *get_or_create_pt(uint32_t *page_dir, uint32_t pd_index) {
    if (page_dir[pd_index] & PAGE_PRESENT) {
        return (uint32_t *)(uint32_t)(page_dir[pd_index] & 0xFFFFF000);
    }

    /* Allocate a new page table */
    uint32_t pt_phys = vm_alloc_page();
    if (!pt_phys) return NULL;

    uint32_t *pt = (uint32_t *)(uint32_t)pt_phys;

    /* Clear the page table */
    for (int i = 0; i < 1024; ++i)
        pt[i] = 0;

    /* Map it in the page directory */
    page_dir[pd_index] = make_pte(pt_phys, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);

    return pt;
}

int vm_map_page(uint32_t *page_dir, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    uint32_t pd_index = PD_INDEX(virt_addr);
    uint32_t pt_index = PT_INDEX(virt_addr);
    uint32_t *pt;

    if (!page_dir) return 0;

    pt = (uint32_t *)(uint32_t)(page_dir[pd_index] & 0xFFFFF000);
    if (!(page_dir[pd_index] & PAGE_PRESENT)) {
        /* Allocate new page table */
        uint32_t pt_phys = vm_alloc_page();
        if (!pt_phys) return 0;

        /* Identity map the page table so we can write to it */
        /* We need to map the page table into our current address space */
        uint32_t *current_pd = (uint32_t *)paging_get_page_directory();
        uint32_t temp_virt = 0xFFC00000;  /* Page table self-map trick */

        /* For now, use a simpler approach: access page table via its physical addr
         * by creating a temporary mapping */
        page_dir[pd_index] = make_pte(pt_phys, PAGE_PRESENT | PAGE_WRITABLE | (flags & PAGE_USER));
        pt = (uint32_t *)(uint32_t)pt_phys;
    } else {
        pt = (uint32_t *)(uint32_t)(page_dir[pd_index] & 0xFFFFF000);
    }

    pt[pt_index] = make_pte(phys_addr, flags);

    /* Flush TLB for this page */
    __asm__ volatile("invlpg %0" ::"m"(*(volatile char *)virt_addr) : "memory");

    return 1;
}

int vm_unmap_page(uint32_t *page_dir, uint32_t virt_addr) {
    uint32_t pd_index = PD_INDEX(virt_addr);
    uint32_t pt_index = PT_INDEX(virt_addr);
    uint32_t *pt;

    if (!page_dir) return 0;
    if (!(page_dir[pd_index] & PAGE_PRESENT)) return 0;

    pt = (uint32_t *)(uint32_t)(page_dir[pd_index] & 0xFFFFF000);
    pt[pt_index] = 0;

    __asm__ volatile("invlpg %0" ::"m"(*(volatile char *)virt_addr) : "memory");
    return 1;
}

uint32_t vm_get_phys_addr(uint32_t *page_dir, uint32_t virt_addr) {
    uint32_t pd_index = PD_INDEX(virt_addr);
    uint32_t pt_index = PT_INDEX(virt_addr);
    uint32_t *pt;

    if (!page_dir) return 0;
    if (!(page_dir[pd_index] & PAGE_PRESENT)) return 0;

    pt = (uint32_t *)(uint32_t)(page_dir[pd_index] & 0xFFFFF000);
    if (!(pt[pt_index] & PAGE_PRESENT)) return 0;

    return (pt[pt_index] & 0xFFFFF000) | (virt_addr & 0xFFF);
}

/* ================================================================
 *  Page Directory Management
 * ================================================================ */

uint32_t *vm_create_page_dir(void) {
    uint32_t pd_phys = vm_alloc_page();
    if (!pd_phys) return NULL;

    uint32_t *pd = (uint32_t *)(uint32_t)pd_phys;

    /* Clear page directory */
    int i;
    for (i = 0; i < 1024; ++i)
        pd[i] = 0;

    return pd;
}

void vm_clone_kernel_space(uint32_t *user_pd) {
    uint32_t *kernel_pd;
    int i;

    /* Get kernel page directory */
    kernel_pd = (uint32_t *)paging_get_page_directory();
    if (!kernel_pd) return;

    /* Clone kernel entries (upper 2GB) */
    for (i = 512; i < 1024; ++i) {
        if (kernel_pd[i] & PAGE_PRESENT) {
            user_pd[i] = kernel_pd[i];
        }
    }
}

void vm_switch_page_dir(uint32_t *page_dir) {
    __asm__ volatile("mov %0, %%cr3" ::"r"(page_dir) : "memory");
}

/* ================================================================
 *  Virtual Memory Region Allocation
 * ================================================================ */

uint32_t vm_alloc_region(uint32_t size, uint32_t flags) {
    uint32_t pages = (size + 4095) / 4096;
    uint32_t addr = 0;
    int i;

    /* Find a free region slot */
    for (i = 0; i < VM_MAX_REGIONS; ++i) {
        if (!g_regions[i].used) break;
    }

    if (i >= VM_MAX_REGIONS) {
        kputs("[VM] ERRO: sem slots de regiao\n");
        return 0;
    }

    /* Find a suitable virtual address */
    uint32_t start_addr;
    if (flags & PAGE_USER)
        start_addr = 0x00400000;  /* User space starts at 4MB */
    else
        start_addr = HEAP_START;  /* Kernel space at 3.5GB */

    /* Simple bump allocator for now */
    if (flags & PAGE_USER) {
        static uint32_t next_user_addr = 0x00400000;
        addr = next_user_addr;
        next_user_addr += pages * 4096;
        if (next_user_addr > 0xBF800000) next_user_addr = 0x00400000;  /* Wrap */
    } else {
        addr = g_heap_brk;
        g_heap_brk += pages * 4096;
    }

    /* Allocate physical pages and map them */
    uint32_t *current_pd = (uint32_t *)paging_get_page_directory();
    uint32_t j;
    for (j = 0; j < pages; ++j) {
        uint32_t phys = vm_alloc_page();
        if (!phys) {
            kputs("[VM] ERRO: sem memoria para alocar regiao\n");
            return 0;
        }
        vm_map_page(current_pd, addr + j * 4096, phys, flags | PAGE_PRESENT);
    }

    g_regions[i].base = addr;
    g_regions[i].size = pages * 4096;
    g_regions[i].type = (flags & PAGE_USER) ? VM_REGION_USER : VM_REGION_KERNEL;
    g_regions[i].flags = flags;
    g_regions[i].used = 1;

    return addr;
}

void vm_free_region(uint32_t base) {
    int i;
    for (i = 0; i < VM_MAX_REGIONS; ++i) {
        if (g_regions[i].used && g_regions[i].base == base) {
            uint32_t pages = g_regions[i].size / 4096;
            uint32_t *pd = (uint32_t *)paging_get_page_directory();
            uint32_t j;

            for (j = 0; j < pages; ++j) {
                uint32_t phys = vm_get_phys_addr(pd, base + j * 4096);
                if (phys) {
                    vm_unmap_page(pd, base + j * 4096);
                    vm_free_page(phys);
                }
            }

            g_regions[i].used = 0;
            return;
        }
    }
}

/* ================================================================
 *  Kernel Heap
 * ================================================================ */

void *vm_kernel_alloc(uint32_t size) {
    uint32_t pages = (size + 4095) / 4096;
    if (pages == 0) pages = 1;

    uint32_t addr = vm_alloc_region(pages * 4096, PAGE_PRESENT | PAGE_WRITABLE);
    if (!addr) return NULL;

    return (void *)addr;
}

void vm_kernel_free(void *ptr) {
    if (ptr)
        vm_free_region((uint32_t)ptr);
}

/* ================================================================
 *  Debug
 * ================================================================ */

void vm_dump_stats(void) {
    kprintf("[VM] Total pages: %u  Used: %u  Free: %u\n",
            g_total_pages, g_used_pages, g_total_pages - g_used_pages);
    kprintf("[VM] Heap brk: 0x%08x\n", g_heap_brk);
    kprintf("[VM] Regions:\n");
    int i;
    for (i = 0; i < VM_MAX_REGIONS; ++i) {
        if (g_regions[i].used) {
            kprintf("  [%d] 0x%08x - 0x%08x (%u bytes) type=%d flags=0x%x\n",
                    i, g_regions[i].base,
                    g_regions[i].base + g_regions[i].size - 1,
                    g_regions[i].size, g_regions[i].type, g_regions[i].flags);
        }
    }
}
