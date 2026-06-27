@echo off
title CoreOS 95 - Gerar ISO
cd /d "%~dp0"

if not exist "C:\msys64\msys2_shell.cmd" (
    echo ERRO: MSYS2 nao encontrado. Instale: https://www.msys2.org/
    pause
    exit /b 1
)

set GCC_OK=0
if exist "tools\i686-elf\bin\i686-elf-gcc.exe" set GCC_OK=1
if exist "tools\toolchain-ready.txt" set GCC_OK=1
for /r tools %%F in (i686-elf-gcc.exe) do set GCC_OK=1

if %GCC_OK%==0 (
    echo.
    echo ERRO: compilador i686-elf nao instalado.
    echo MinGW NAO funciona para o kernel!
    echo.
    echo 1. Rode: instalar-toolchain.bat
    echo    OU leia INSTALAR-MANUAL.txt
    echo 2. Depois rode este arquivo de novo.
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo   CoreOS 95 - Compilar + Gerar ISO
echo ========================================
echo.

C:\msys64\msys2_shell.cmd -mingw32 -defterm -no-start -here -c "bash ./build.sh iso"
set BUILD_EXIT=%ERRORLEVEL%

echo.
echo ========================================
if %BUILD_EXIT% NEQ 0 (
    echo   FALHOU na compilacao.
) else if exist "build\x86\coreos.iso" (
    echo   SUCESSO!
    echo.
    echo   ISO: %~dp0build\x86\coreos.iso
    echo   IMG: %~dp0build\x86\coreos.img
    echo.
    echo   VirtualBox - ISO ^(unidade CD, NAO disco rigido^):
    echo   Configuracoes - Armazenamento - icone CD vazio
    echo   - Escolher arquivo coreos.iso
    echo   Boot: Optico primeiro, EFI DESLIGADO
    echo.
    echo   MAIS FACIL - disco rigido:
    echo   Use coreos.img ou rode CRIAR-VDI.bat -^> coreos.vdi
) else if exist "build\x86\coreos.img" (
    echo   Disco gerado ^(ISO opcional nao criada^):
    echo   %~dp0build\x86\coreos.img
    echo.
    echo   Para ISO: no MSYS2 MINGW32 rode: pacman -S cdrtools
    echo   VirtualBox: use o .img como disco IDE ^(recomendado^).
) else (
    echo   FALHOU - nenhum arquivo gerado.
)
echo ========================================
echo.
pause
