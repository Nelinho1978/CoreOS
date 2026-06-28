#include "ntos/ps.h"
#include "ntos/mm.h"
#include "coreos/printk.h"
#include "arch/irq.h"

/* Global process/thread tables */
static EPROCESS *g_ProcessList = NULL;
static KTHREAD *g_ThreadList = NULL;
static EPROCESS *g_SystemProcess = NULL;
static KTHREAD *g_CurrentThread = NULL;

static uint32_t g_NextPid = 1;
static uint32_t g_NextTid = 1;

/* Process management */
static EPROCESS g_ProcessTable[PS_MAX_PROCESSES];
static KTHREAD g_ThreadTable[PS_MAX_THREADS];
static int g_ProcessCount = 0;
static int g_ThreadCount = 0;

void PspInitSystem(void) {
    kputs("[Ps] Process Manager initializing...\n");
    
    /* Create System process (PID 4) */
    g_SystemProcess = &g_ProcessTable[0];
    g_SystemProcess->ProcessId = 4;
    g_SystemProcess->ImageFileName = "System";
    g_SystemProcess->ThreadCount = 0;
    g_SystemProcess->CreateTime = 0;
    g_SystemProcess->NextProcess = NULL;
    g_ProcessCount++;
    g_ProcessList = g_SystemProcess;
    
    /* Create system idle thread */
    KTHREAD *idle = &g_ThreadTable[0];
    idle->ThreadId = 1;
    idle->Priority = 0;
    idle->State = ThreadReady;
    idle->QuantumRemaining = 10;
    idle->StackSize = 4096;
    idle->StackBase = MmAllocatePool(idle->StackSize);
    idle->Process = g_SystemProcess;
    idle->NextThread = NULL;
    g_ThreadCount++;
    g_ThreadList = idle;
    g_CurrentThread = idle;
    
    g_SystemProcess->PrimaryThread = idle;
    g_SystemProcess->ThreadList = idle;
    g_SystemProcess->ThreadCount = 1;
    
    kprintf("[Ps] System process created (PID %u)\n", g_SystemProcess->ProcessId);
    kprintf("[Ps] Idle thread created (TID %u)\n", idle->ThreadId);
}

EPROCESS *PsGetCurrentProcess(void) {
    return g_CurrentThread ? g_CurrentThread->Process : g_SystemProcess;
}

EPROCESS *PsGetInitialSystemProcess(void) {
    return g_SystemProcess;
}

KTHREAD *PsGetCurrentThread(void) {
    return g_CurrentThread;
}

void PsSetCurrentThread(KTHREAD *Thread) {
    if (Thread) {
        g_CurrentThread = Thread;
    }
}

NTSTATUS PsCreateProcess(PCSTR ImagePath, EPROCESS **OutProcess) {
    if (!ImagePath || !OutProcess)
        return STATUS_INVALID_PARAMETER;
    
    if (g_ProcessCount >= PS_MAX_PROCESSES)
        return STATUS_NO_MEMORY;
    
    EPROCESS *proc = &g_ProcessTable[g_ProcessCount];
    proc->ProcessId = g_NextPid++;
    proc->ImageFileName = ImagePath;
    proc->ThreadCount = 0;
    proc->ThreadList = NULL;
    proc->PrimaryThread = NULL;
    proc->CreateTime = 0;
    proc->VirtualAddressSpace = MmCreateAddressSpace();
    proc->ExitCode = 0;
    proc->NextProcess = NULL;
    
    /* Link to process list */
    if (g_ProcessList) {
        EPROCESS *p = g_ProcessList;
        while (p->NextProcess) p = p->NextProcess;
        p->NextProcess = proc;
    }
    
    g_ProcessCount++;
    *OutProcess = proc;
    
    kprintf("[Ps] Process created: %s (PID %u)\n", ImagePath, proc->ProcessId);
    return STATUS_SUCCESS;
}

NTSTATUS PsCreateThread(EPROCESS *Process, uint32_t EntryPoint, void *StartContext, KTHREAD **OutThread) {
    if (!Process || !OutThread)
        return STATUS_INVALID_PARAMETER;
    
    if (g_ThreadCount >= PS_MAX_THREADS)
        return STATUS_NO_MEMORY;
    
    KTHREAD *thread = &g_ThreadTable[g_ThreadCount];
    thread->ThreadId = g_NextTid++;
    thread->Priority = 8;  /* Normal priority */
    thread->State = ThreadReady;
    thread->QuantumRemaining = 10;
    thread->StackSize = 8192;
    thread->StackBase = MmAllocatePool(thread->StackSize);
    thread->Process = Process;
    thread->NextThread = NULL;
    
    /* Initialize thread context */
    thread->Context.esp = thread->StackBase + thread->StackSize - 4;
    thread->Context.eip = EntryPoint;
    thread->Context.eflags = 0x202;  /* IF=1, reserved bits */
    thread->Context.ebp = thread->StackBase + thread->StackSize;
    
    /* Store start context on stack */
    uint32_t *stack = (uint32_t *)thread->Context.esp;
    *stack = (uint32_t)StartContext;
    
    /* Link to process thread list */
    if (!Process->ThreadList) {
        Process->ThreadList = thread;
        Process->PrimaryThread = thread;
    } else {
        KTHREAD *t = Process->ThreadList;
        while (t->NextThread) t = t->NextThread;
        t->NextThread = thread;
    }
    Process->ThreadCount++;
    
    /* Link to global thread list */
    if (g_ThreadList) {
        KTHREAD *t = g_ThreadList;
        while (t->NextThread) t = t->NextThread;
        t->NextThread = thread;
    }
    
    g_ThreadCount++;
    *OutThread = thread;
    
    kprintf("[Ps] Thread created (TID %u, PID %u, Entry=0x%X)\n", thread->ThreadId, Process->ProcessId, EntryPoint);
    return STATUS_SUCCESS;
}

NTSTATUS PsTerminateProcess(EPROCESS *Process, uint32_t ExitCode) {
    if (!Process)
        return STATUS_INVALID_PARAMETER;
    
    kprintf("[Ps] Terminating process PID %u (ExitCode=%u)\n", Process->ProcessId, ExitCode);
    
    /* Terminate all threads */
    KTHREAD *thread = Process->ThreadList;
    while (thread) {
        thread->State = ThreadTerminated;
        thread = thread->NextThread;
    }
    
    Process->ExitCode = ExitCode;
    return STATUS_SUCCESS;
}

NTSTATUS PsTerminateThread(KTHREAD *Thread, uint32_t ExitCode) {
    if (!Thread)
        return STATUS_INVALID_PARAMETER;
    
    Thread->State = ThreadTerminated;
    kprintf("[Ps] Thread terminated (TID %u, ExitCode=%u)\n", Thread->ThreadId, ExitCode);
    return STATUS_SUCCESS;
}

/* Simple round-robin scheduler */
void PsScheduler(void) {
    if (!g_CurrentThread)
        return;
    
    /* Decrement quantum */
    if (g_CurrentThread->QuantumRemaining > 0)
        g_CurrentThread->QuantumRemaining--;
    
    /* If quantum expired, find next ready thread */
    if (g_CurrentThread->QuantumRemaining == 0 || g_CurrentThread->State != ThreadRunning) {
        KTHREAD *next = g_CurrentThread->NextThread;
        
        /* Find next ready thread */
        while (next && next->State != ThreadReady && next->State != ThreadRunning) {
            next = next->NextThread;
        }
        
        /* Wrap around */
        if (!next)
            next = g_ThreadList;
        
        /* Find ready thread from beginning */
        while (next && next->State != ThreadReady && next->State != ThreadRunning) {
            next = next->NextThread;
        }
        
        if (next && next != g_CurrentThread) {
            g_CurrentThread->State = ThreadReady;
            next->State = ThreadRunning;
            next->QuantumRemaining = 10;
            g_CurrentThread = next;
        }
    }
}
