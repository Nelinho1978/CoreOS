#include "ntos/io.h"
#include "coreos/printk.h"

extern void DrvVideoRegister(void);
extern void DrvKeyboardRegister(void);
extern void DrvMouseRegister(void);

void IoLoadBuiltinDrivers(void) {
    DrvVideoRegister();
    DrvKeyboardRegister();
    DrvMouseRegister();
    kputs("[Io] Drivers integrados carregados (Video, Keyboard, Mouse)\n");
}
