#include "coreos/startup.h"
#include "coreos/kernel.h"
#include "coreos/firmware.h"
#include "gui/win10.h"
#include "ntos/ntoskrnl.h"
#include "coreos/printk.h"
#include "coreos/drivers_hw.h"
#include "coreos/debug_console.h"
#include "ntos/ldr.h"
#include "subsys/win32_stubs.h"
#include "arch/irq.h"
#include "arch/vm.h"
#include "arch/scheduler.h"
#include "arch/ata.h"
#include "arch/paging.h"

/* External declarations for new features */
extern int fat32_init(void);
extern int fat32_list_root(void);

void criar_memoria_virtual(uint32_t boot_magic, void *boot_info) {
    (void)boot_magic;
    (void)boot_info;
    kputs("[Kernel] CriarMemoriaVirtual\n");

    /* Initialize the page tables (already done in paging_init) */
    /* Now initialize the virtual memory manager with dynamic allocation */
    vm_init();

    /* Also initialize the debug serial console */
    debug_console_init();
}

void criar_scheduler(void) {
    kputs("[Kernel] CriarScheduler\n");
    KeInitSystem();

    /* Initialize the real preemptive scheduler */
    sched_init();
}

void criar_drivers(void) {
    kputs("[Kernel] CriarDrivers\n");
    IoInitSystem();
    IoLoadBuiltinDrivers();
    drivers_inicializar_hardware();

    /* Initialize ATA/IDE disk driver */
    kputs("[Kernel] Inicializando disco...\n");
    if (ata_init()) {
        kputs("[Kernel] Disco ATA detectado\n");
        /* Try to initialize FAT32 */
        if (fat32_init()) {
            kputs("[Kernel] FAT32 pronto\n");
            fat32_list_root();
        }
    } else {
        kputs("[Kernel] Disco ATA nao detectado\n");
    }
}

void criar_processos(void) {
    kputs("[Kernel] CriarProcessos\n");
    MmInitSystem(COREOS_BOOT_MAGIC_DIRECT, (void *)0);
    ObInitSystem();
    SeInitSystem();
    CmInitSystem();
    ExInitSystem();
    PspInitSystem();
    LdrInitSystem();
    Win32StubsInit();
    KePhase1Init();

    /* Now that IDT is loaded, start the PIT timer for preemptive scheduling */
    {
        extern void pit_init_sched(uint32_t frequency_hz);
        pit_init_sched(100);
    }
}

void mostrar_tela(void) {
    const firmware_state_t *fw = firmware_estado();

    kputs("[Kernel] MostrarTela\n");
    if (!fw->video_ok) {
        kputs("[Kernel] ERRO: video nao disponivel\n");
        return;
    }

    LdrLoadDll(NULL, "ntdll.dll");
    LdrLoadDll(NULL, "kernel32.dll");
    LdrLoadDll(NULL, "user32.dll");
    LdrLoadDll(NULL, "gdi32.dll");

    /* Run the desktop */
    desktop_run();
}

void kernel_executar(uint32_t boot_magic, void *boot_info) {
    (void)boot_info;

    kputs("=== KernelMain ===\n");

    criar_memoria_virtual(boot_magic, boot_info);
    criar_scheduler();
    criar_drivers();
    criar_processos();
    mostrar_tela();

    for (;;) {
        __asm__ volatile("pause");
    }
}
