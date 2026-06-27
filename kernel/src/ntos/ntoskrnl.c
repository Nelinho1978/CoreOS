#include "ntos/ntoskrnl.h"
#include "subsys/win32.h"
#include "coreos/printk.h"
#include "hal/hal.h"

NTSTATUS NTAPI NtInitializeExecutive(uint32_t boot_magic, void *boot_info) {
    kputs("\n=== CoreOS NT Kernel (arquitetura Windows 10) ===\n");

    KeInitSystem();
    MmInitSystem(boot_magic, boot_info);
    ObInitSystem();
    SeInitSystem();
    CmInitSystem();
    ExInitSystem();
    PspInitSystem();
    IoInitSystem();
    IoLoadBuiltinDrivers();

    hal_late_init();
    KePhase1Init();

    if (!NT_SUCCESS(Win32SubsystemInitialize())) {
        kputs("[NT] Falha ao iniciar subsistema Win32\n");
        return STATUS_NOT_IMPLEMENTED;
    }

    Win32SessionManagerStart();
    return STATUS_SUCCESS;
}

void NTAPI NtShutdownSystem(void) {
    kputs("[NT] Shutdown solicitado\n");
    hal_halt();
}
