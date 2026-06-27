#ifndef COREOS_ARCH_X86_VM_H
#define COREOS_ARCH_X86_VM_H

#include "coreos/types.h"

/* Page table flags */
#define PAGE_PRESENT    0x01
#define PAGE_WRITABLE   0x02
#define PAGE_USER       0x04
#define PAGE_WRITETHRU  0x08
#define PAGE_CACHE_DIS  0x10
#define PAGE_ACCESSED   0x20
#define PAGE_DIRTY      0x40
#define PAGE_SIZE_4MB   0x80
#define PAGE_GLOBAL     0x100

/* Memory region types */
#define VM_REGION_FREE      0
#define VM_REGION_KERNEL    1
#define VM_REGION_USER      2
#define VM_REGION_MMIO      3
#define VM_REGION_PAGED     4

/* Max physical memory pages tracked */
#define VM_MAX_PAGES    32768

/* Page directory/table entry */
typedef uint32_t page_entry_t;

/* Memory region descriptor */
typedef struct {
    uint32_t base;       /* Virtual base address */
    uint32_t size;       /* Size in bytes */
    uint32_t type;       /* VM_REGION_* */
    uint32_t flags;      /* PAGE_* flags */
    int      used;
} VM_REGION;

/* ---- Initialization ---- */

/* Initialize virtual memory manager with page tables */
void vm_init(void);

/* ---- Physical Page Management ---- */

/* Mark a physical page as used */
void vm_mark_page_used(uint32_t phys_addr);

/* Mark a range as used (from memory map) */
void vm_mark_range_used(uint32_t base, uint32_t size);

/* Allocate a physical page */
uint32_t vm_alloc_page(void);

/* Free a physical page */
void vm_free_page(uint32_t phys_addr);

/* ---- Virtual Memory Mapping ---- */

/* Map a physical page to a virtual address with flags */
int vm_map_page(uint32_t *page_dir, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);

/* Unmap a virtual page */
int vm_unmap_page(uint32_t *page_dir, uint32_t virt_addr);

/* Get physical address from virtual */
uint32_t vm_get_phys_addr(uint32_t *page_dir, uint32_t virt_addr);

/* ---- Page Directory Management ---- */

/* Create a new page directory (for a process) */
uint32_t *vm_create_page_dir(void);

/* Clone kernel space into a user page directory */
void vm_clone_kernel_space(uint32_t *user_pd);

/* Switch to a page directory */
void vm_switch_page_dir(uint32_t *page_dir);

/* ---- Allocation ---- */

/* Allocate virtual memory region */
uint32_t vm_alloc_region(uint32_t size, uint32_t flags);

/* Free a virtual memory region */
void vm_free_region(uint32_t base);

/* ---- Kernel Heap ---- */

/* Simple kernel heap allocator (page-based) */
void *vm_kernel_alloc(uint32_t size);
void vm_kernel_free(void *ptr);

/* ---- Debug ---- */

/* Dump page table state */
void vm_dump_stats(void);

#endif
