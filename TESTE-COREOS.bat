@echo off
setlocal EnableExtensions
title Teste CoreOS 10
cd /d "%~dp0"

set "ROOT=%~dp0"
set "KERNEL=%ROOT%kernel"
set "IMG=%KERNEL%\build\x86\coreos.img"
set "LOG=%KERNEL%\build\x86\serial.log"
set "SHOT=%KERNEL%\build\x86\teste-desktop.png"
set "VMNAME=CoreOS 10"

set "VBM="
if exist "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe" set "VBM=C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"
if "%VBM%"=="" if exist "C:\Program Files (x86)\Oracle\VirtualBox\VBoxManage.exe" set "VBM=C:\Program Files (x86)\Oracle\VirtualBox\VBoxManage.exe"

if "%VBM%"=="" (
    echo [ERRO] VirtualBox nao encontrado.
    exit /b 1
)

if not exist "C:\msys64\msys2_shell.cmd" (
    echo [ERRO] MSYS2 nao encontrado.
    exit /b 1
)

echo.
echo === [1/4] Compilando ===
"%VBM%" controlvm "%VMNAME%" poweroff >nul 2>&1
timeout /t 1 /nobreak >nul
cd /d "%KERNEL%"
C:\msys64\msys2_shell.cmd -mingw32 -defterm -no-start -here -c "bash ./build.sh"
if errorlevel 1 (
    echo [ERRO] Compilacao falhou.
    exit /b 1
)
if not exist "%IMG%" (
    echo [ERRO] coreos.img nao gerado.
    exit /b 1
)
echo [OK] %IMG%

echo.
echo === [2/4] VM de teste ===
"%VBM%" list vms | findstr /C:"\"%VMNAME%\"" >nul 2>&1
if errorlevel 1 (
    call "%ROOT%COREOS.bat" >nul 2>&1
    "%VBM%" controlvm "%VMNAME%" poweroff >nul 2>&1
    timeout /t 2 /nobreak >nul
)

if exist "%LOG%" del /f "%LOG%" >nul 2>&1

"%VBM%" modifyvm "%VMNAME%" --uart1 0x3F8 4 --uartmode1 file "%LOG%"
"%VBM%" modifyvm "%VMNAME%" --audio-driver default --audio-enabled on --audio-controller ac97
"%VBM%" modifyvm "%VMNAME%" --nic1 nat --nictype1 82540EM
"%VBM%" storageattach "%VMNAME%" --storagectl "Floppy" --port 0 --device 0 --type fdd --medium "%IMG%" >nul 2>&1

echo.
echo === [3/4] Boot headless (25s) ===
"%VBM%" startvm "%VMNAME%" --type headless
timeout /t 25 /nobreak >nul
"%VBM%" controlvm "%VMNAME%" screenshotpng "%SHOT%" >nul 2>&1
"%VBM%" controlvm "%VMNAME%" poweroff >nul 2>&1
timeout /t 2 /nobreak >nul

echo.
echo === [4/4] Resultado ===
if exist "%SHOT%" (
    echo [OK] Screenshot: %SHOT%
) else (
    echo [FALHOU] Screenshot nao gerado.
)

set "PASS=1"
if not exist "%LOG%" (
    echo [FALHOU] Log serial nao encontrado: %LOG%
    set "PASS=0"
) else (
    echo.
    echo --- Log serial ---
    type "%LOG%"
    echo --- fim log ---
    echo.

    findstr /C:"[VIDEO]" "%LOG%" >nul 2>&1
    if errorlevel 1 (
        echo [FALHOU] Sem linha [VIDEO] no log.
        set "PASS=0"
    ) else (
        echo [OK] Video no log.
    )

    findstr /C:"[REDE]" "%LOG%" >nul 2>&1
    if errorlevel 1 (
        echo [FALHOU] Driver de rede nao reportou [REDE]
        set "PASS=0"
    ) else (
        echo [OK] Rede no log.
    )

    findstr /C:"[SOM]" "%LOG%" >nul 2>&1
    if errorlevel 1 (
        echo [FALHOU] Driver de som nao reportou [SOM]
        set "PASS=0"
    ) else (
        echo [OK] Som no log.
    )

    findstr /C:"Relatorio de drivers" "%LOG%" >nul 2>&1
    if errorlevel 1 (
        echo [AVISO] Relatorio de drivers nao encontrado.
    ) else (
        echo [OK] Relatorio de drivers.
    )
)

if "%PASS%"=="1" (
    echo.
    echo ========================================
    echo  TESTE PASSOU
    echo ========================================
    exit /b 0
) else (
    echo.
    echo ========================================
    echo  TESTE FALHOU — ver log e screenshot
    echo ========================================
    exit /b 1
)
