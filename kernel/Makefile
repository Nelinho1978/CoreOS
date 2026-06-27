# CoreOS 95 — Kernel Makefile
# Boot DIRETO: MBR proprio carrega o kernel do disco (sem GRUB).
#
#   make        — compila kernel
#   make disk   — gera imagem de disco bootavel
#   make run    — reinicia direto no QEMU a partir do disco

ARCH ?= x86
TARGET = coreos-kernel
BUILD_DIR = build/$(ARCH)

INC = -Iinclude -Iarch/x86/include
CFLAGS_COMMON = -ffreestanding -fno-stack-protector -fno-pic -Wall -Wextra -O2 $(INC)
LDFLAGS_COMMON = -nostdlib -static

CORE_SRC = \
	src/core/main.c \
	src/core/firmware.c \
	src/core/startup.c \
	src/core/panic.c \
	src/core/printk.c \
	src/core/memory.c \
	src/core/boot_menu.c \
	src/core/setup.c \
	src/core/drivers_hw.c \
	src/gui/fb.c \
	src/gui/font8x16.c \
	src/gui/boot_gfx.c \
	src/gui/desktop.c \
	src/gfx/gfx_accel.c \
	src/gfx/raster.c \
	src/gfx/opengl_sw.c \
	src/gfx/directx_sw.c \
	src/gfx/display_detect.c \
	src/hal/hal.c \
	src/ntos/ntoskrnl.c \
	src/ntos/ke/keinit.c \
	src/ntos/ex/exinit.c \
	src/ntos/ob/ob.c \
	src/ntos/io/iomgr.c \
	src/ntos/io/ioinit.c \
	src/ntos/mm/mminit.c \
	src/ntos/ps/psinit.c \
	src/ntos/se/seinit.c \
	src/ntos/cm/cminit.c \
	src/ntos/ldr/pe_load.c \
	src/ntos/ldr/ldr.c \
	src/subsys/win32/win32k.c \
	src/subsys/win32/stubs.c

.PHONY: all clean run disk iso dirs

ifeq ($(ARCH),x86)

CC = gcc
AS = nasm
LD = ld
OBJCOPY = objcopy
CFLAGS = $(CFLAGS_COMMON) -m32 -std=gnu99 -fno-builtin
ASFLAGS_ELF = -f elf32
ASFLAGS_BIN = -f bin
LDFLAGS = $(LDFLAGS_COMMON) -m elf_i386 -T arch/x86/linker.ld

ARCH_SRC = arch/x86/hal.c arch/x86/video/vga.c arch/x86/video/svga.c arch/x86/video/bochs_svga.c arch/x86/cpu/cpuid.c \
	arch/x86/mem/ram_probe.c arch/x86/bus/pci.c arch/x86/io/serial.c \
	arch/x86/time/pit.c arch/x86/ke/gdt.c arch/x86/ke/syscall.c arch/x86/ke/irq.c \
	arch/x86/drivers/drvvga.c arch/x86/drivers/drvkbd.c arch/x86/drivers/drvmou.c \
	arch/x86/drivers/e1000.c arch/x86/drivers/ac97.c \
	arch/x86/input/keyboard.c arch/x86/input/mouse.c arch/x86/usb/ohci.c \
	arch/x86/usb/uhci.c arch/x86/usb/usb_stack.c arch/x86/mm/paging.c
ARCH_ASM = arch/x86/boot/boot.S arch/x86/ke/irq_asm.S
STAGE2_SRC = arch/x86/boot/stage2.S
MBR_SRC = arch/x86/boot/mbr.S

KERNEL_ELF = $(BUILD_DIR)/$(TARGET).elf
KERNEL_FLAT = $(BUILD_DIR)/$(TARGET).flat
STAGE2_BIN = $(BUILD_DIR)/stage2.bin
MBR_BIN = $(BUILD_DIR)/mbr.bin
DISK_IMG = $(BUILD_DIR)/coreos.img
ISO_FILE = $(BUILD_DIR)/coreos.iso
STAGE2_SECTORS = 8
KERNEL_LBA = 9
BOOT_SECTORS = 65536

C_SRC = $(CORE_SRC) $(ARCH_SRC)
C_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRC))
ASM_OBJ = $(patsubst %.S,$(BUILD_DIR)/%.o,$(ARCH_ASM))
OBJS = $(ASM_OBJ) $(C_OBJ)

all: dirs $(KERNEL_FLAT)

$(KERNEL_ELF): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

$(KERNEL_FLAT): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $< $@
	@echo "Kernel flat: $@ ($(shell wc -c < $@) bytes)"

$(MBR_BIN): $(MBR_SRC)
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS_BIN) $< -o $@

$(DISK_IMG): $(MBR_BIN) $(STAGE2_BIN) $(KERNEL_FLAT)
	@mkdir -p $(BUILD_DIR)
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none 2>/dev/null || \
		dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	dd if=$(MBR_BIN) of=$@ conv=notrunc bs=512 count=1 status=none 2>/dev/null || \
		dd if=$(MBR_BIN) of=$@ conv=notrunc bs=512 count=1
	dd if=$(STAGE2_BIN) of=$@ conv=notrunc bs=512 seek=1 status=none 2>/dev/null || \
		dd if=$(STAGE2_BIN) of=$@ conv=notrunc bs=512 seek=1
	dd if=$(KERNEL_FLAT) of=$@ conv=notrunc bs=512 seek=$(KERNEL_LBA) status=none 2>/dev/null || \
		dd if=$(KERNEL_FLAT) of=$@ conv=notrunc bs=512 seek=$(KERNEL_LBA)
	@echo "Disco bootavel: $@"

$(STAGE2_BIN): $(STAGE2_SRC)
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS_BIN) $< -o $@

disk: $(DISK_IMG)

# ISO bootavel (El Torito) para VirtualBox / gravar em CD
iso: $(ISO_FILE)

$(ISO_FILE): $(DISK_IMG)
	@if command -v genisoimage >/dev/null 2>&1; then \
		genisoimage -o $@ \
			-b $(notdir $(DISK_IMG)) \
			-c boot.cat \
			-no-emul-boot \
			-boot-load-size $(BOOT_SECTORS) \
			-boot-info-table \
			$(BUILD_DIR); \
	elif command -v mkisofs >/dev/null 2>&1; then \
		mkisofs -o $@ \
			-b $(notdir $(DISK_IMG)) \
			-c boot.cat \
			-no-emul-boot \
			-boot-load-size $(BOOT_SECTORS) \
			-boot-info-table \
			$(BUILD_DIR); \
	elif command -v xorriso >/dev/null 2>&1; then \
		xorriso -as mkisofs -o $@ \
			-b $(notdir $(DISK_IMG)) \
			-c boot.cat \
			-no-emul-boot \
			-boot-load-size $(BOOT_SECTORS) \
			-boot-info-table \
			$(BUILD_DIR); \
	else \
		echo "ERRO: instale genisoimage, mkisofs ou xorriso"; \
		echo "  sudo apt install genisoimage"; \
		exit 1; \
	fi
	@echo "ISO bootavel: $@"

run: disk
	qemu-system-i386 \
		-drive file=$(DISK_IMG),format=raw,index=0,if=ide \
		-serial stdio \
		-no-reboot

dirs:
	@mkdir -p $(BUILD_DIR)/src/core $(BUILD_DIR)/src/gui $(BUILD_DIR)/src/gfx $(BUILD_DIR)/src/hal
	@mkdir -p $(BUILD_DIR)/src/ntos/ke $(BUILD_DIR)/src/ntos/ex $(BUILD_DIR)/src/ntos/ob
	@mkdir -p $(BUILD_DIR)/src/ntos/io $(BUILD_DIR)/src/ntos/mm $(BUILD_DIR)/src/ntos/ps
	@mkdir -p $(BUILD_DIR)/src/ntos/se $(BUILD_DIR)/src/ntos/cm $(BUILD_DIR)/src/subsys/win32
	@mkdir -p $(BUILD_DIR)/arch/x86/boot $(BUILD_DIR)/arch/x86/video $(BUILD_DIR)/arch/x86/cpu
	@mkdir -p $(BUILD_DIR)/arch/x86/time $(BUILD_DIR)/arch/x86/ke $(BUILD_DIR)/arch/x86/drivers
	@mkdir -p $(BUILD_DIR)/arch/x86/input $(BUILD_DIR)/arch/x86/mm $(BUILD_DIR)/arch/x86/usb

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS_ELF) $< -o $@

else ifeq ($(ARCH),arm)

CC ?= gcc
CFLAGS = $(CFLAGS_COMMON)
all: dirs
	@echo "==> ARCH=arm: HAL stub (boot pendente)"
	@mkdir -p $(BUILD_DIR)/arch/arm
	$(CC) $(CFLAGS) -c arch/arm/hal.c -o $(BUILD_DIR)/arch/arm/hal.o
	@echo "HAL ARM compilado."

dirs:
	@mkdir -p $(BUILD_DIR)/arch/arm

disk run:
	@echo "Boot ARM ainda nao implementado."

else ifeq ($(ARCH),riscv)

CC ?= gcc
CFLAGS = $(CFLAGS_COMMON)
all: dirs
	@echo "==> ARCH=riscv: HAL stub (boot pendente)"
	@mkdir -p $(BUILD_DIR)/arch/riscv
	$(CC) $(CFLAGS) -c arch/riscv/hal.c -o $(BUILD_DIR)/arch/riscv/hal.o
	@echo "HAL RISC-V compilado."

dirs:
	@mkdir -p $(BUILD_DIR)/arch/riscv

disk run:
	@echo "Boot RISC-V ainda nao implementado."

else

$(error ARCH invalida: $(ARCH). Use x86, arm ou riscv)

endif

clean:
	rm -rf build
