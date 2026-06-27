#include "arch/scheduler.h"
#include "arch/ports.h"
#include "coreos/printk.h"

/* ---- Internal state ---- */
static SCHED_PROCESS g_processes[SCHED_MAX_PROCS];
static SCHED_THREAD  g_threads[SCHED_MAX_THREADS];
static uint32_t g_current_thread;
static uint32_t g_next_pid = 1;
static uint32_t g_next_tid = 1;
static int g_sched_initialized;
TSS_ENTRY g_tss __attribute__((aligned(4)));

/* ---- PIT timer (for preemption) ---- */
void pit_init_sched(uint32_t frequency_hz) {
    uint32_t divisor = 1193180 / frequency_hz;

    kprintf("[PIT] Inicializando timer com %u Hz\n", frequency_hz);

    outb(0x43, 0x36);  /* Channel 0, Lobyte/Hibyte, Square Wave */
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

/* ---- TSS management ---- */
void tss_init(void) {
    int i;
    uint8_t *tss_bytes = (uint8_t *)&g_tss;

    /* Zero out TSS */
    for (i = 0; i < sizeof(TSS_ENTRY); ++i)
        tss_bytes[i] = 0;

    g_tss.ss0 = 0x10;               /* Kernel data segment */
    g_tss.esp0 = 0x90000;           /* Initial kernel stack */
    g_tss.cs = 0x08 | 0x03;         /* Ring 3 code segment */
    g_tss.ss = 0x10 | 0x03;         /* Ring 3 data segment */
    g_tss.ds = 0x10 | 0x03;
    g_tss.es = 0x10 | 0x03;
    g_tss.fs = 0x10 | 0x03;
    g_tss.gs = 0x10 | 0x03;
    g_tss.io_map_base = sizeof(TSS_ENTRY);

    kputs("[TSS] Inicializado\n");
}

void tss_set_stack(uint32_t esp0, uint16_t ss0) {
    g_tss.esp0 = esp0;
    g_tss.ss0 = ss0;
}

uint32_t tss_get_phys_addr(void) {
    return (uint32_t)&g_tss;
}

/* ---- Internal: find free process/thread slot ---- */
static SCHED_PROCESS *find_free_process(void) {
    int i;
    for (i = 0; i < SCHED_MAX_PROCS; ++i) {
        if (!g_processes[i].used)
            return &g_processes[i];
    }
    return NULL;
}

static SCHED_THREAD *find_free_thread(void) {
    int i;
    for (i = 0; i < SCHED_MAX_THREADS; ++i) {
        if (!g_threads[i].used)
            return &g_threads[i];
    }
    return NULL;
}

/* ---- Initialize scheduler ---- */
void sched_init(void) {
    int i;

    if (g_sched_initialized) return;

    kputs("[Sched] Inicializando scheduler...\n");

    /* Clear process list */
    for (i = 0; i < SCHED_MAX_PROCS; ++i)
        g_processes[i].used = 0;

    /* Clear thread list */
    for (i = 0; i < SCHED_MAX_THREADS; ++i)
        g_threads[i].used = 0;

    /* Create idle/system process */
    {
        SCHED_PROCESS *proc = find_free_process();
        if (proc) {
            proc->pid = g_next_pid++;
            proc->name[0] = 'S'; proc->name[1] = 'y'; proc->name[2] = 's';
            proc->name[3] = 't'; proc->name[4] = 'e'; proc->name[5] = 'm';
            proc->name[6] = '\0';
            proc->state = PROC_STATE_READY;
            proc->page_dir = 0;
            proc->used = 1;
        }
    }

    /* Create idle thread */
    {
        SCHED_THREAD *thread = find_free_thread();
        if (thread) {
            thread->tid = g_next_tid++;
            thread->pid = 1;
            thread->state = THREAD_STATE_READY;
            thread->esp = 0;
            thread->eip = 0;
            thread->kernel_stack = 0x90000;
            thread->user_stack = 0;
            thread->used = 1;
            thread->quanta = 5;
        }
    }

    g_current_thread = 0;

    /* Initialize TSS */
    tss_init();
    tss_set_stack(0x90000, 0x10);

    /* NOTE: PIT init moved to after KePhase1Init (IDT must be loaded first) */

    g_sched_initialized = 1;
    kputs("[Sched] Scheduler pronto\n");
}

/* ---- Process creation ---- */
int sched_create_process(const char *name, uint32_t entry, uint32_t page_dir) {
    SCHED_PROCESS *proc;

    if (!g_sched_initialized) return 0;

    proc = find_free_process();
    if (!proc) {
        kputs("[Sched] ERRO: maximo de processos\n");
        return 0;
    }

    proc->pid = g_next_pid++;
    {
        int i;
        for (i = 0; i < 31 && name && name[i]; ++i)
            proc->name[i] = name[i];
        proc->name[31] = '\0';
    }
    proc->state = PROC_STATE_READY;
    proc->page_dir = page_dir;
    proc->base_addr = 0;
    proc->size = 0;
    proc->used = 1;

    /* Create main thread */
    sched_create_thread(proc->pid, entry, 0);

    kprintf("[Sched] Processo criado: %s (PID %u)\n", name, proc->pid);
    return proc->pid;
}

/* ---- Thread creation ---- */
int sched_create_thread(uint32_t pid, uint32_t entry, uint32_t user_stack) {
    SCHED_THREAD *thread;
    SCHED_PROCESS *proc = NULL;
    int i;

    if (!g_sched_initialized) return 0;

    /* Find parent process */
    for (i = 0; i < SCHED_MAX_PROCS; ++i) {
        if (g_processes[i].used && g_processes[i].pid == pid) {
            proc = &g_processes[i];
            break;
        }
    }
    if (!proc) return 0;

    thread = find_free_thread();
    if (!thread) {
        kputs("[Sched] ERRO: maximo de threads\n");
        return 0;
    }

    thread->tid = g_next_tid++;
    thread->pid = pid;
    thread->state = THREAD_STATE_CREATED;
    thread->user_stack = user_stack;
    thread->used = 1;
    thread->quanta = 5;

    /* Allocate kernel stack for this thread */
    thread->kernel_stack = 0xE0000000;  /* Placeholder */
    thread->esp = 0;
    thread->eip = entry;

    return thread->tid;
}

/* ---- Simple round-robin scheduler ---- */
void sched_switch(void) {
    uint32_t next = g_current_thread;
    uint32_t tries = SCHED_MAX_THREADS;

    if (!g_sched_initialized) return;

    /* Find next READY thread (round-robin) */
    while (tries--) {
        next = (next + 1) % SCHED_MAX_THREADS;
        if (g_threads[next].used && g_threads[next].state == THREAD_STATE_READY)
            break;
    }

    if (next == g_current_thread)
        return;  /* No other thread to run */

    if (!g_threads[next].used)
        return;

    /* Save current ESP */
    uint32_t old_esp;
    __asm__ volatile("mov %%esp, %0" : "=r"(old_esp));

    g_threads[g_current_thread].esp = old_esp;
    g_threads[g_current_thread].state = THREAD_STATE_READY;

    /* Switch to new thread */
    g_current_thread = next;
    g_threads[next].state = THREAD_STATE_RUNNING;

    /* Set kernel stack in TSS */
    tss_set_stack(g_threads[next].kernel_stack, 0x10);

    /* Context switch */
    __asm__ volatile(
        "mov %0, %%esp\n\t"
        : : "r"(g_threads[next].esp) : "memory"
    );

    /* Will continue execution at new thread's EIP when scheduled */
}

void sched_yield(void) {
    __asm__ volatile("int $0x2E" ::: "memory");  /* Trigger syscall -> scheduler */
}

uint32_t sched_get_current_thread(void) {
    if (!g_sched_initialized) return 1;
    return g_threads[g_current_thread].tid;
}

uint32_t sched_get_current_process(void) {
    if (!g_sched_initialized) return 4;
    return g_threads[g_current_thread].pid;
}

void sched_timer_tick(void) {
    /* Called from IRQ0 - PIT timer tick */
    SCHED_THREAD *thread;

    if (!g_sched_initialized) return;

    thread = &g_threads[g_current_thread];
    if (!thread->used) return;

    if (thread->quanta > 0) {
        thread->quanta--;
        return;  /* Still has time left */
    }

    /* Time quanta expired - switch to next thread */
    thread->quanta = 5;  /* Reset quanta */
    sched_switch();
}
