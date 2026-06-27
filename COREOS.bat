@echo off
setlocal EnableExtensions
title CoreOS 10
cd /d "%~dp0"

rem ============================================================
rem  COREOS.BAT — unico script: compilar + VM VirtualBox + iniciar
rem  (Drivers NVIDIA reais nao existem para este kernel; usamos
rem   VBE/SVGA emulado pelo VirtualBox — VBoxVGA)
rem ============================================================

set "ROOT=%~dp0"
set "KERNEL=%ROOT%kernel"
set "IMG=%KERNEL%\build\x86\coreos.img"
set "LOG=%KERNEL%\build\x86\serial.log"
set "VMNAME=CoreOS 10"

set "VBM="
if exist "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe" set "VBM=C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"
if "%VBM%"=="" if exist "C:\Program Files (x86)\Oracle\VirtualBox\VBoxManage.exe" set "VBM=C:\Program Files (x86)\Oracle\VirtualBox\VBoxManage.exe"

if "%VBM%"=="" (
    echo [ERRO] VirtualBox nao encontrado. Instale: https://www.virtualbox.org/
    pause
    exit /b 1
)

if not exist "C:\msys64\msys2_shell.cmd" (
    echo [ERRO] MSYS2 nao encontrado. Instale: https://www.msys2.org/
    echo Depois: pacman -S mingw-w64-i686-nasm
    pause
    exit /b 1
)

echo.
echo === [1/3] Compilando CoreOS 10 ===
"%VBM%" controlvm "%VMNAME%" poweroff >nul 2>&1
timeout /t 1 /nobreak >nul

cd /d "%KERNEL%"
C:\msys64\msys2_shell.cmd -mingw32 -defterm -no-start -here -c "bash ./build.sh"
if errorlevel 1 (
    echo [ERRO] Compilacao falhou.
    pause
    exit /b 1
)
if not exist "build\x86\coreos.img" (
    echo [ERRO] coreos.img nao foi gerado.
    pause
    exit /b 1
)
echo [OK] Disco: %IMG%

echo.
echo === [2/3] Configurando VM VirtualBox ===
for %%V in ("CoreOS 10" "Core_OS_10" "CoreOS 95" "CoreOs") do (
    "%VBM%" controlvm %%V poweroff >nul 2>&1
)
timeout /t 2 /nobreak >nul
for %%V in ("CoreOS 10" "Core_OS_10" "CoreOS 95" "CoreOs") do (
    "%VBM%" unregistervm %%V --delete >nul 2>&1
)

"%VBM%" createvm --name "%VMNAME%" --ostype "Other" --register
if errorlevel 1 goto :vmfail

rem VBoxVGA: VBE estavel neste kernel (VMSVGA pode causar triple fault)
"%VBM%" modifyvm "%VMNAME%" --graphicscontroller vboxvga
"%VBM%" modifyvm "%VMNAME%" --memory 128 --vram 128
"%VBM%" modifyvm "%VMNAME%" --firmware bios
"%VBM%" modifyvm "%VMNAME%" --accelerate3d off
"%VBM%" modifyvm "%VMNAME%" --usb on --usbehci off --usbohci on
"%VBM%" modifyvm "%VMNAME%" --mouse ps2 --keyboard ps2
"%VBM%" modifyvm "%VMNAME%" --acpi on --ioapic off
"%VBM%" modifyvm "%VMNAME%" --boot1 floppy --boot2 none --boot3 none --boot4 none
"%VBM%" modifyvm "%VMNAME%" --pae off --longmode off --cpus 1
"%VBM%" modifyvm "%VMNAME%" --uart1 0x3F8 4 --uartmode1 file "%LOG%"
"%VBM%" modifyvm "%VMNAME%" --audio-driver default --audio-enabled on --audio-controller ac97
"%VBM%" modifyvm "%VMNAME%" --nic1 nat --nictype1 82540EM
"%VBM%" modifyvm "%VMNAME%" --chipset piix3

"%VBM%" storagectl "%VMNAME%" --name "Floppy" --add floppy --controller I82078
"%VBM%" storageattach "%VMNAME%" --storagectl "Floppy" --port 0 --device 0 --type fdd --medium "%IMG%"
if errorlevel 1 goto :vmfail
echo [OK] VM "%VMNAME%" criada (VBoxVGA 800x600, USB OHCI, 82540EM, AC97, log serial)

echo.
echo === [3/3] Iniciando ===
echo   Clique DENTRO da janela da VM para o mouse funcionar.
echo.
start "" "C:\Program Files\Oracle\VirtualBox\VirtualBox.exe"
"%VBM%" startvm "%VMNAME%" --type gui
exit /b 0

:vmfail
echo [ERRO] Falha ao criar/configurar a VM.
pause
exit /b 1
