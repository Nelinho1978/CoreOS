#include "ntos/ps.h"
#include "coreos/printk.h"

static EPROCESS g_system_process;
static KTHREAD g_system_thread;

void PspInitSystem(void) {
    g_system_thread.Header.Type = ObTypeThread;
    g_system_thread.Header.RefCount = 1;
    g_system_thread.Header.Name[0] = '\0';
    g_system_thread.ThreadId = 1;
    g_system_thread.Priority = 8;

    g_system_process.Header.Type = ObTypeProcess;
    g_system_process.Header.RefCount = 1;
    g_system_process.Header.Name[0] = 'S';
    g_system_process.Header.Name[1] = 'y';
    g_system_process.Header.Name[2] = 's';
    g_system_process.Header.Name[3] = 't';
    g_system_process.Header.Name[4] = 'e';
    g_system_process.Header.Name[5] = 'm';
    g_system_process.Header.Name[6] = '\0';
    g_system_process.ProcessId = 4;
    g_system_process.ImageFileName = "System";
    g_system_process.PrimaryThread = &g_system_thread;

    kputs("[Ps] Process Manager: processo System (PID 4)\n");
}

EPROCESS *PsGetCurrentProcess(void) {
    return &g_system_process;
}

EPROCESS *PsGetInitialSystemProcess(void) {
    return &g_system_process;
}
