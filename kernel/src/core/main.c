#include "coreos/kernel.h"
#include "coreos/firmware.h"
#include "coreos/startup.h"
#include "hal/hal.h"
#include "arch/irq.h"

extern const hal_interface_t *hal_arch_get_interface(void);

void kernel_main(uint32_t magic, void *boot_info) {
    const hal_interface_t *iface = hal_arch_get_interface();

    x86_irq_disable_all();
    hal_register(iface);

    /* Firmware: DescobrirHardware + ConfigurarHardware */
    firmware_iniciar();

    /* Kernel: CarregarKernel ja feito pelo stage2 — agora ExecutarKernel */
    kernel_executar(magic, boot_info);
}
