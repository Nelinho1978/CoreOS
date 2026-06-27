#include "coreos/startup.h"
#include "coreos/kernel.h"
#include "coreos/firmware.h"
#include "gui/win10.h"
#include "ntos/ntoskrnl.h"
#include "coreos/printk.h"
#include "coreos/drivers_hw.h"
#include "arch/irq.h"

void criar_memoria_virtual(uint32_t boot_magic, void *boot_info) {
    (void)boot_magic;
    (void)boot_info;
    kputs("[Kernel] CriarMemoriaVirtual\n");
    /* Paginacao e mapa ja preparados em inicializar_video / MmInit */
}

void criar_scheduler(void) {
    kputs("[Kernel] CriarScheduler\n");
    KeInitSystem();
}

void criar_drivers(void) {
    kputs("[Kernel] CriarDrivers\n");
    IoInitSystem();
    IoLoadBuiltinDrivers();
    drivers_inicializar_hardware();
}

void criar_processos(void) {
    kputs("[Kernel] CriarProcessos\n");
    MmInitSystem(COREOS_BOOT_MAGIC_DIRECT, (void *)0);
    ObInitSystem();
    SeInitSystem();
    CmInitSystem();
    ExInitSystem();
    PspInitSystem();
    KePhase1Init();
}

void mostrar_tela(void) {
    const firmware_state_t *fw = firmware_estado();

    kputs("[Kernel] MostrarTela\n");
    if (!fw->video_ok) {
        kputs("[Kernel] ERRO: video nao disponivel\n");
        return;
    }
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
