@echo off
title CoreOS 95 - Criar VDI para VirtualBox
cd /d "%~dp0"

if not exist "build\x86\coreos.img" (
    echo ERRO: coreos.img nao encontrado. Rode COMPILAR.bat primeiro.
    pause
    exit /b 1
)

set VBM=
if exist "C:\Program Files\Oracle\VirtualBox\VBoxManage.exe" (
    set "VBM=C:\Program Files\Oracle\VirtualBox\VBoxManage.exe"
)
if exist "C:\Program Files (x86)\Oracle\VirtualBox\VBoxManage.exe" (
    set "VBM=C:\Program Files (x86)\Oracle\VirtualBox\VBoxManage.exe"
)

if "%VBM%"=="" (
    echo.
    echo VBoxManage nao encontrado.
    echo.
    echo Use coreos.img diretamente no VirtualBox:
    echo   Configuracoes - Armazenamento - IDE - Adicionar disco rígido
    echo   Arquivo: build\x86\coreos.img
    echo   Filtro: Todos os arquivos (*.*)
    echo.
    pause
    exit /b 1
)

echo Convertendo coreos.img para coreos.vdi ...
"%VBM%" convertfromraw "build\x86\coreos.img" "build\x86\coreos.vdi" --format VDI

if %ERRORLEVEL% NEQ 0 (
    echo FALHOU na conversao.
    pause
    exit /b 1
)

echo.
echo SUCESSO!
echo   %~dp0build\x86\coreos.vdi
echo.
echo No VirtualBox: anexe coreos.vdi como disco IDE.
echo EFI desligado.
echo.
pause
