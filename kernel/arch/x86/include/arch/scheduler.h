#ifndef COREOS_ARCH_X86_SCHEDULER_H
#define COREOS_ARCH_X86_SCHEDULER_H

#include "coreos/types.h"

/* Maximum processes and threads */
#define SCHED_MAX_PROCS  16
#define SCHED_MAX_THREADS 32
#define SCHED_STACK_SIZE 8192    /* 8KB per thread stack */

/* Process states */
#define PROC_STATE_READY     0
#define PROC_STATE_RUNNING   1
#define PROC_STATE_BLOCKED   2
#define PROC_STATE_TERMINATED 3

/* Thread states */
#define THREAD_STATE_READY   0
#define THREAD_STATE_RUNNING 1
#define THREAD_STATE_BLOCKED 2
#define THREAD_STATE_TERMINATED 3
#define THREAD_STATE_CREATED 4

/* Process structure */
typedef struct _SCHED_PROCESS {
    uint32_t  pid;
    char      name[32];
    uint32_t  state;
    uint32_t  page_dir;       /* Physical address of page directory */
    uint32_t  base_addr;      /* Base address of process image */
    uint32_t  size;           /* Size of process image */
    int       used;
} SCHED_PROCESS;

/* Thread/context structure */
typedef struct _SCHED_THREAD {
    uint32_t  tid;
    uint32_t  pid;            /* Parent process */
    uint32_t  state;
    uint32_t  esp;            /* Kernel stack pointer (saved) */
    uint32_t  ebp;            /* Saved EBP */
    uint32_t  eip;            /* Saved EIP */
    uint32_t  kernel_stack;   /* Top of kernel stack */
    uint32_t  user_stack;     /* User-mode stack */
    int       used;
    int       quanta;         /* Remaining time quanta */
} SCHED_THREAD;

/* x86 TSS structure (Task State Segment) */
typedef struct __attribute__((packed)) {
    uint16_t prev_task;
    uint16_t reserved0;
    uint32_t esp0;            /* Kernel stack pointer (ring 0) */
    uint16_t ss0;             /* Kernel stack segment (ring 0) */
    uint16_t reserved1;
    uint32_t esp1;
    uint16_t ss1;
    uint16_t reserved2;
    uint32_t esp2;
    uint16_t ss2;
    uint16_t reserved3;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint16_t reserved4;
    uint32_t cs;
    uint16_t reserved5;
    uint32_t ss;
    uint16_t reserved6;
    uint32_t ds;
    uint16_t reserved7;
    uint32_t fs;
    uint16_t reserved8;
    uint32_t gs;
    uint16_t reserved9;
    uint16_t ldt;
    uint16_t reserved10;
    uint16_t io_map_base;
} TSS_ENTRY;

/* ---- Scheduler API ---- */

/* Initialize scheduler (TSS, PIT, idle thread) */
void sched_init(void);

/* Create a new process */
int sched_create_process(const char *name, uint32_t entry, uint32_t page_dir);

/* Create a new thread in a process */
int sched_create_thread(uint32_t pid, uint32_t entry, uint32_t user_stack);

/* Yield the current thread (voluntary) */
void sched_yield(void);

/* Get current thread ID */
uint32_t sched_get_current_thread(void);

/* Get current process ID */
uint32_t sched_get_current_process(void);

/* Switch to next thread (called by timer or yield) */
void sched_switch(void);

/* IRQ handler for timer tick (preemption) */
void sched_timer_tick(void);

/* ---- Context Switch (assembly) ---- */
void sched_context_switch(uint32_t *old_esp, uint32_t new_esp);

/* ---- TSS management ---- */
void tss_set_stack(uint32_t esp0, uint16_t ss0);
void tss_init(void);

/* ---- PIT timer ---- */
void pit_init(uint32_t frequency_hz);

#endif
