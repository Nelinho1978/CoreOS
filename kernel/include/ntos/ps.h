#ifndef COREOS_NTOS_PS_H
#define COREOS_NTOS_PS_H

#include "nt/ntdef.h"
#include "ntos/ob.h"
#include "ntos/mm.h"

#define PS_MAX_PROCESSES 64
#define PS_MAX_THREADS 256

/* Thread states */
typedef enum _THREAD_STATE {
    ThreadInitialized = 0,
    ThreadReady = 1,
    ThreadRunning = 2,
    ThreadBlocked = 3,
    ThreadTerminated = 4
} THREAD_STATE;

/* Thread context (x86 registers) */
typedef struct _CONTEXT {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cr3;  /* Page directory */
} CONTEXT;

/* Kernel Thread */
typedef struct _KTHREAD {
    OBJECT_HEADER Header;
    ULONG ThreadId;
    ULONG Priority;
    THREAD_STATE State;
    CONTEXT Context;
    uint32_t QuantumRemaining;
    uint32_t StackBase;
    uint32_t StackSize;
    struct _KTHREAD *NextThread;
    struct _EPROCESS *Process;
} KTHREAD;

/* Executive Process */
typedef struct _EPROCESS {
    OBJECT_HEADER Header;
    ULONG ProcessId;
    PCSTR ImageFileName;
    KTHREAD *PrimaryThread;
    KTHREAD *ThreadList;
    uint32_t ThreadCount;
    uint32_t VirtualAddressSpace;
    uint32_t ExitCode;
    uint32_t CreateTime;
    uint32_t EntryPoint;
    struct _EPROCESS *NextProcess;
} EPROCESS;

/* Public API */
void PspInitSystem(void);
EPROCESS *PsGetCurrentProcess(void);
EPROCESS *PsGetInitialSystemProcess(void);
NTSTATUS PsCreateProcess(PCSTR ImagePath, EPROCESS **OutProcess);
NTSTATUS PsCreateThread(EPROCESS *Process, uint32_t EntryPoint, void *StartContext, KTHREAD **OutThread);
NTSTATUS PsTerminateProcess(EPROCESS *Process, uint32_t ExitCode);
NTSTATUS PsTerminateThread(KTHREAD *Thread, uint32_t ExitCode);
void PsScheduler(void);
KTHREAD *PsGetCurrentThread(void);
void PsSetCurrentThread(KTHREAD *Thread);

#endif
