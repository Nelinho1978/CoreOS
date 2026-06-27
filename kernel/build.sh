#!/bin/bash
# CoreOS 10 - build Windows (requer i686-elf, NAO MinGW)
set -e

TARGET="${1:-disk}"

cd "$(dirname "$0")"
BUILD=build/x86

# Limpar build anterior (evita misturar objetos MinGW COFF com ELF)
rm -rf "$BUILD"
mkdir -p "$BUILD"/{src/core,src/gui,src/gfx,src/hal,src/ntos/{ke,ex,ob,io,mm,ps,se,cm,ldr},src/subsys/win32,arch/x86/{boot,video,cpu,time,ke,drivers,input,mm,mem,bus,io,usb}}

# Localizar toolchain i686-elf
TOOLCHAIN_BIN=""
if [ -f "tools/toolchain-ready.txt" ]; then
  TOOLCHAIN_BIN="$(tr -d '\r' < tools/toolchain-ready.txt)"
fi
if [ -z "$TOOLCHAIN_BIN" ] || [ ! -f "$TOOLCHAIN_BIN/i686-elf-gcc.exe" ]; then
  GCC_FOUND=$(find tools -name "i686-elf-gcc.exe" 2>/dev/null | head -1)
  if [ -n "$GCC_FOUND" ]; then
    TOOLCHAIN_BIN="$(dirname "$GCC_FOUND")"
  fi
fi

if [ -n "$TOOLCHAIN_BIN" ] && [ -f "$TOOLCHAIN_BIN/i686-elf-gcc.exe" ]; then
  export PATH="$TOOLCHAIN_BIN:$PATH"
  echo "Usando toolchain: $TOOLCHAIN_BIN"
fi

if command -v i686-elf-gcc &>/dev/null; then
  CC=i686-elf-gcc
  LD=i686-elf-ld
  OBJCOPY=i686-elf-objcopy
else
  echo ""
  echo "ERRO: i686-elf-gcc nao encontrado."
  echo "MinGW (i686-w64-mingw32-gcc) NAO serve para o kernel."
  echo ""
  echo "Instale a toolchain:"
  echo "  1. Duplo clique: instalar-toolchain.bat"
  echo "  OU leia: INSTALAR-MANUAL.txt"
  echo "  Link: https://github.com/lordmilko/i686-elf-tools/releases/tag/7.1.0"
  echo ""
  exit 1
fi

if ! command -v nasm &>/dev/null; then
  echo "ERRO: nasm nao encontrado. No MSYS2 MINGW32: pacman -S nasm"
  exit 1
fi

CFLAGS="-ffreestanding -fno-stack-protector -fno-pic -fno-builtin -std=gnu99 -Wall -Wextra -O2 -Iinclude -Iarch/x86/include"

STAGE2_SECTORS=8
KERNEL_LBA=9

echo "Compilando kernel (i686-elf)..."

compile_c() {
  local src=$1
  local obj=$BUILD/${src%.c}.o
  mkdir -p "$(dirname "$obj")"
  echo "  CC  $src"
  $CC $CFLAGS -c "$src" -o "$obj"
}

compile_c src/core/main.c
compile_c src/core/firmware.c
compile_c src/core/startup.c
compile_c src/core/panic.c
compile_c src/core/printk.c
compile_c src/core/memory.c
compile_c src/core/boot_menu.c
compile_c src/core/setup.c
compile_c src/core/drivers_hw.c
compile_c src/core/debug_console.c
compile_c src/gui/fb.c
compile_c src/gui/font8x16.c
compile_c src/gui/boot_gfx.c
compile_c src/gui/desktop.c
compile_c src/gfx/gfx_accel.c
compile_c src/gfx/raster.c
compile_c src/gfx/opengl_sw.c
compile_c src/gfx/directx_sw.c
compile_c src/gfx/display_detect.c
compile_c src/hal/hal.c
compile_c src/ntos/ntoskrnl.c
compile_c src/ntos/ke/keinit.c
compile_c src/ntos/ex/exinit.c
compile_c src/ntos/ob/ob.c
compile_c src/ntos/io/iomgr.c
compile_c src/ntos/io/ioinit.c
compile_c src/ntos/io/fat32.c
compile_c src/ntos/mm/mminit.c
compile_c src/ntos/ps/psinit.c
compile_c src/ntos/se/seinit.c
compile_c src/ntos/cm/cminit.c
compile_c src/ntos/ldr/pe_load.c
compile_c src/ntos/ldr/ldr.c
compile_c src/subsys/win32/win32k.c
compile_c src/subsys/win32/stubs.c
compile_c arch/x86/hal.c
compile_c arch/x86/video/vga.c
compile_c arch/x86/video/svga.c
compile_c arch/x86/video/bochs_svga.c
compile_c arch/x86/cpu/cpuid.c
compile_c arch/x86/mem/ram_probe.c
compile_c arch/x86/bus/pci.c
compile_c arch/x86/io/serial.c
compile_c arch/x86/time/pit.c
compile_c arch/x86/ke/gdt.c
compile_c arch/x86/ke/syscall.c
compile_c arch/x86/ke/irq.c
compile_c arch/x86/ke/sched.c
compile_c arch/x86/drivers/drvvga.c
compile_c arch/x86/drivers/drvkbd.c
compile_c arch/x86/drivers/drvmou.c
compile_c arch/x86/drivers/e1000.c
compile_c arch/x86/drivers/ac97.c
compile_c arch/x86/drivers/ata_pio.c
compile_c arch/x86/input/keyboard.c
compile_c arch/x86/input/mouse.c
compile_c arch/x86/usb/ohci.c
compile_c arch/x86/usb/uhci.c
compile_c arch/x86/usb/usb_stack.c
compile_c arch/x86/mm/paging.c
compile_c arch/x86/mm/vm.c

echo "  AS  arch/x86/boot/stage2.S"
nasm -f bin arch/x86/boot/stage2.S -o "$BUILD/stage2.bin"

echo "  AS  arch/x86/boot/boot.S"
nasm -f elf32 arch/x86/boot/boot.S -o "$BUILD/arch/x86/boot/boot.o"

echo "  AS  arch/x86/ke/irq_asm.S"
nasm -f elf32 arch/x86/ke/irq_asm.S -o "$BUILD/arch/x86/ke/irq_asm.o"

echo "  AS  arch/x86/ke/sched_asm.S"
nasm -f elf32 arch/x86/ke/sched_asm.S -o "$BUILD/arch/x86/ke/sched_asm.o"

echo "  AS  arch/x86/boot/mbr.S"
nasm -f bin arch/x86/boot/mbr.S -o "$BUILD/mbr.bin"

OBJS=(
  "$BUILD/arch/x86/boot/boot.o"
  "$BUILD/src/core/main.o"
  "$BUILD/src/core/firmware.o"
  "$BUILD/src/core/startup.o"
  "$BUILD/src/core/panic.o"
  "$BUILD/src/core/printk.o"
  "$BUILD/src/core/memory.o"
  "$BUILD/src/core/boot_menu.o"
  "$BUILD/src/core/setup.o"
  "$BUILD/src/core/drivers_hw.o"
  "$BUILD/src/core/debug_console.o"
  "$BUILD/src/gui/fb.o"
  "$BUILD/src/gui/font8x16.o"
  "$BUILD/src/gui/boot_gfx.o"
  "$BUILD/src/gui/desktop.o"
  "$BUILD/src/gfx/gfx_accel.o"
  "$BUILD/src/gfx/raster.o"
  "$BUILD/src/gfx/opengl_sw.o"
  "$BUILD/src/gfx/directx_sw.o"
  "$BUILD/src/gfx/display_detect.o"
  "$BUILD/src/hal/hal.o"
  "$BUILD/src/ntos/ntoskrnl.o"
  "$BUILD/src/ntos/ke/keinit.o"
  "$BUILD/src/ntos/ex/exinit.o"
  "$BUILD/src/ntos/ob/ob.o"
  "$BUILD/src/ntos/io/iomgr.o"
  "$BUILD/src/ntos/io/ioinit.o"
  "$BUILD/src/ntos/io/fat32.o"
  "$BUILD/src/ntos/mm/mminit.o"
  "$BUILD/src/ntos/ps/psinit.o"
  "$BUILD/src/ntos/se/seinit.o"
  "$BUILD/src/ntos/cm/cminit.o"
  "$BUILD/src/ntos/ldr/pe_load.o"
  "$BUILD/src/ntos/ldr/ldr.o"
  "$BUILD/src/subsys/win32/win32k.o"
  "$BUILD/src/subsys/win32/stubs.o"
  "$BUILD/arch/x86/hal.o"
  "$BUILD/arch/x86/video/vga.o"
  "$BUILD/arch/x86/video/svga.o"
  "$BUILD/arch/x86/video/bochs_svga.o"
  "$BUILD/arch/x86/cpu/cpuid.o"
  "$BUILD/arch/x86/mem/ram_probe.o"
  "$BUILD/arch/x86/bus/pci.o"
  "$BUILD/arch/x86/io/serial.o"
  "$BUILD/arch/x86/time/pit.o"
  "$BUILD/arch/x86/ke/gdt.o"
  "$BUILD/arch/x86/ke/syscall.o"
  "$BUILD/arch/x86/ke/irq.o"
  "$BUILD/arch/x86/ke/sched_asm.o"
  "$BUILD/arch/x86/ke/sched.o"
  "$BUILD/arch/x86/ke/irq_asm.o"
  "$BUILD/arch/x86/drivers/drvvga.o"
  "$BUILD/arch/x86/drivers/drvkbd.o"
  "$BUILD/arch/x86/drivers/drvmou.o"
  "$BUILD/arch/x86/drivers/e1000.o"
  "$BUILD/arch/x86/drivers/ac97.o"
  "$BUILD/arch/x86/drivers/ata_pio.o"
  "$BUILD/arch/x86/input/keyboard.o"
  "$BUILD/arch/x86/input/mouse.o"
  "$BUILD/arch/x86/usb/ohci.o"
  "$BUILD/arch/x86/usb/uhci.o"
  "$BUILD/arch/x86/usb/usb_stack.o"
  "$BUILD/arch/x86/mm/paging.o"
  "$BUILD/arch/x86/mm/vm.o"
)

echo "  LD  coreos-kernel.elf"
$LD -m elf_i386 -nostdlib -static -T arch/x86/linker.ld "${OBJS[@]}" -o "$BUILD/coreos-kernel.elf"

$OBJCOPY -O binary "$BUILD/coreos-kernel.elf" "$BUILD/coreos-kernel.flat"
KERNEL_BYTES=$(wc -c < "$BUILD/coreos-kernel.flat")
KERNEL_NEED=$(( (KERNEL_BYTES + 511) / 512 ))
echo "Kernel flat: $BUILD/coreos-kernel.flat ($KERNEL_BYTES bytes, $KERNEL_NEED setores)"
if [ "$KERNEL_NEED" -gt 256 ]; then
  echo "ERRO: kernel maior que 256 setores (128 KB). Aumente KERNEL_SECTORS no stage2.S"
  exit 1
fi

DISK_SECTORS=2880

IMG="$BUILD/coreos.img"
dd if=/dev/zero of="$IMG" bs=512 count=$DISK_SECTORS status=none 2>/dev/null || dd if=/dev/zero of="$IMG" bs=512 count=$DISK_SECTORS
dd if="$BUILD/mbr.bin" of="$IMG" conv=notrunc bs=512 count=1 status=none
dd if="$BUILD/stage2.bin" of="$IMG" conv=notrunc bs=512 seek=1 status=none
dd if="$BUILD/coreos-kernel.flat" of="$IMG" conv=notrunc bs=512 seek=$KERNEL_LBA status=none

echo ""
echo "Disco: $IMG"

ISO="$BUILD/coreos.iso"
PY_CMD=""
if command -v python3 &>/dev/null; then PY_CMD="python3"
elif command -v python &>/dev/null; then PY_CMD="python"
elif command -v py &>/dev/null; then PY_CMD="py -3"
fi

if [ -n "$PY_CMD" ]; then
  echo "  ISO python (scripts/make_iso.py)..."
  $PY_CMD scripts/make_iso.py "$IMG" "$ISO"
  if [ -f "$ISO" ]; then
    echo "ISO: $ISO"
  fi
elif [ "$TARGET" = "iso" ]; then
  ISO_OK=0
  if command -v mkisofs &>/dev/null; then
    echo "  ISO mkisofs..."
    mkisofs -o "$ISO" -b coreos.img -c boot.cat -no-emul-boot -boot-load-size 2880 -boot-info-table "$BUILD"
    ISO_OK=1
  elif command -v genisoimage &>/dev/null; then
    echo "  ISO genisoimage..."
    genisoimage -o "$ISO" -b coreos.img -c boot.cat -no-emul-boot -boot-load-size 2880 -boot-info-table "$BUILD"
    ISO_OK=1
  else
    PY_CMD=""
    if command -v python3 &>/dev/null; then PY_CMD="python3"
    elif command -v python &>/dev/null; then PY_CMD="python"
    elif command -v py &>/dev/null; then PY_CMD="py -3"
    fi
    if [ -n "$PY_CMD" ]; then
      echo "  ISO python (scripts/make_iso.py)..."
      $PY_CMD scripts/make_iso.py "$IMG" "$ISO"
      ISO_OK=1
    fi
  fi

  if [ "$ISO_OK" = 1 ] && [ -f "$ISO" ]; then
    echo "ISO: $ISO"
  else
    echo ""
    echo "AVISO: nao foi possivel gerar ISO."
    echo "Use o disco .img no VirtualBox (recomendado):"
    echo "  $IMG"
  fi
fi

echo ""
echo "SUCESSO!"
