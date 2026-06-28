#include "ntos/ntoskrnl.h"
#include "ntos/ldr.h"
#include "ntos/syscall.h"
#include "ntos/fat32.h"
#include "subsys/win32.h"
#include "subsys/win32_stubs.h"
#include "coreos/printk.h"
#include "hal/hal.h"

extern int ata_init(void);

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
    
    /* NOVO: Inicializar ATA driver antes de carregar drivers */
    kputs("[NT] Initializing storage subsystem...\n");
    if (!ata_init()) {
        kputs("[NT] WARNING: ATA initialization failed\n");
    } else {
        kputs("[NT] ATA controller ready\n");
        
        /* Inicializar FAT32 filesystem */
        if (!fat32_init()) {
            kputs("[NT] WARNING: FAT32 initialization failed\n");
        } else {
            kputs("[NT] FAT32 filesystem ready\n");
        }
    }
    
    IoLoadBuiltinDrivers();
    
    /* NOVO: Inicializar syscall interface */
    kputs("[NT] Initializing syscall interface...\n");
    SyscallInit();
    kputs("[NT] Syscall interface ready\n");
    
    LdrInitSystem();
    Win32StubsInit();

    hal_late_init();
    KePhase1Init();

    /* Pre-load system DLL stubs */
    {
        extern void *LdrLoadDll(const char *path, const char *name);
        LdrLoadDll(NULL, "ntdll.dll");
        LdrLoadDll(NULL, "kernel32.dll");
        LdrLoadDll(NULL, "user32.dll");
        LdrLoadDll(NULL, "gdi32.dll");
    }

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
