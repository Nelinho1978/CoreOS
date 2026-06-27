#include "nt/ntstatus.h"
#include "nt/ntdef.h"
#include "coreos/printk.h"

/* Ldr imports */
extern void *LdrLoadDll(const char *path, const char *name);
extern void *LdrGetProcedureAddress(void *module, const char *func);
extern void *LdrGetModuleHandle(const char *name);
extern NTSTATUS LdrUnloadDll(void *handle);

extern uint32_t ke_syscall_dispatch(uint32_t syscall_number, uint32_t arg0, uint32_t arg1, uint32_t arg2);

/* Syscall number definitions */
#define SYSCALL_NOP                 0
#define SYSCALL_NT_QUERY_SYSTEM     1
#define SYSCALL_NT_LOAD_DLL         2
#define SYSCALL_NT_GET_PROC_ADDR    3
#define SYSCALL_NT_GET_MODULE_HANDLE 4
#define SYSCALL_NT_FREE_LIBRARY     5
#define SYSCALL_NT_VIRTUAL_ALLOC    6
#define SYSCALL_NT_VIRTUAL_FREE     7
#define SYSCALL_SCHED_YIELD         8

uint32_t ke_syscall_dispatch(uint32_t syscall_number,
                              uint32_t arg0, uint32_t arg1, uint32_t arg2) {

    switch (syscall_number) {
    case SYSCALL_NOP:
        return (uint32_t)STATUS_SUCCESS;

    case SYSCALL_NT_QUERY_SYSTEM:
        kputs("[Nt] NtQuerySystemInformation (stub)\n");
        return (uint32_t)STATUS_SUCCESS;

    case SYSCALL_NT_LOAD_DLL: {
        /* NtLoadDll(dll_name_ptr, dll_path_ptr) */
        const char *dll_name = (const char *)arg0;
        const char *dll_path = arg1 ? (const char *)arg1 : NULL;
        void *handle = LdrLoadDll(dll_path, dll_name);
        kputs("[Nt] NtLoadDll: ");
        kputs(dll_name ? dll_name : "(null)");
        kputs(" -> handle=0x");
        {
            char hex[12];
            uint32_t addr = (uint32_t)handle;
            int j;
            for (j = 0; j < 8; ++j) {
                uint32_t nibble = (addr >> (28 - j*4)) & 0xF;
                hex[j] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
            }
            hex[8] = '\0';
            kputs(hex);
        }
        kputs("\n");
        return (uint32_t)handle;
    }

    case SYSCALL_NT_GET_PROC_ADDR: {
        /* NtGetProcAddress(module_handle, func_name_ptr) */
        void *module = (void *)arg0;
        const char *func_name = (const char *)arg1;
        void *addr = LdrGetProcedureAddress(module, func_name);
        return (uint32_t)addr;
    }

    case SYSCALL_NT_GET_MODULE_HANDLE: {
        /* NtGetModuleHandle(module_name_ptr) */
        const char *name = (const char *)arg0;
        void *handle = LdrGetModuleHandle(name);
        return (uint32_t)handle;
    }

    case SYSCALL_NT_FREE_LIBRARY: {
        /* NtFreeLibrary(module_handle) */
        void *module = (void *)arg0;
        NTSTATUS status = LdrUnloadDll(module);
        return (uint32_t)status;
    }

    case SYSCALL_NT_VIRTUAL_ALLOC: {
        /* NtVirtualAlloc(size, type, protect) */
        extern void *MmAllocatePool(uint32_t bytes);
        uint32_t size = arg0;
        (void)arg1; (void)arg2;
        void *ptr = MmAllocatePool(size);
        kputs("[Nt] NtVirtualAlloc: ");
        {
            char buf[12];
            uint32_t sz = size;
            int j;
            for (j = 9; j >= 0; --j) {
                buf[j] = '0' + (sz % 10);
                sz /= 10;
                if (sz == 0) { buf[10] = '\0'; break; }
            }
            kputs(buf);
        }
        kputs(" bytes -> ");
        {
            char hex[12];
            uint32_t addr = (uint32_t)ptr;
            int j;
            for (j = 0; j < 8; ++j) {
                uint32_t nibble = (addr >> (28 - j*4)) & 0xF;
                hex[j] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
            }
            hex[8] = '\0';
            kputs(hex);
        }
        kputs("\n");
        return (uint32_t)ptr;
    }

    case SYSCALL_NT_VIRTUAL_FREE: {
        /* NtVirtualFree(ptr, size, type) */
        extern void MmFreePool(void *ptr);
        void *ptr = (void *)arg0;
        MmFreePool(ptr);
        return (uint32_t)STATUS_SUCCESS;
    }

    case SYSCALL_SCHED_YIELD: {
        /* NtYieldExecution - yield to next thread */
        extern void sched_switch(void);
        sched_switch();
        return (uint32_t)STATUS_SUCCESS;
    }

    default:
        return (uint32_t)STATUS_NOT_IMPLEMENTED;
    }
}

void ke_irq_handler_c(void) {
    /* IRQ generica — drivers em modo polling por enquanto */
}
