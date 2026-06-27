# CoreOS 10 - Root Makefile
# This Makefile delegates all targets to kernel/Makefile

KERNEL_DIR := kernel

.PHONY: all disk run clean iso help

all: disk

disk:
	$(MAKE) -C $(KERNEL_DIR) disk

run:
	$(MAKE) -C $(KERNEL_DIR) run

clean:
	$(MAKE) -C $(KERNEL_DIR) clean

iso:
	$(MAKE) -C $(KERNEL_DIR) iso

help:
	@echo "CoreOS 10 - Alvo de Compilação"
	@echo "================================"
	@echo ""
	@echo "Comandos disponíveis:"
	@echo "  make          - Compila o kernel e gera imagem de disco"
	@echo "  make disk     - Gera imagem de disco bootável (coreos.img)"
	@echo "  make run      - Executa no QEMU"
	@echo "  make iso      - Gera ISO bootável"
	@echo "  make clean    - Limpa arquivos compilados"
	@echo ""
	@echo "Pré-requisitos:"
	@echo "  - i686-elf-gcc (cross-compiler) ou gcc com multilib"
	@echo "  - nasm"
	@echo "  - qemu-system-i386 (para make run)"
	@echo ""
	@echo "Windows (MSYS2 MINGW32):"
	@echo "  Execute: kernel\\COMPILAR.bat"
	@echo ""
	@echo "Saída:"
	@echo "  kernel/build/x86/coreos-kernel.flat  - Kernel binário"
	@echo "  kernel/build/x86/coreos.img          - Imagem de disco"
