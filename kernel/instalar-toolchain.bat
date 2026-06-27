@echo off
title CoreOS 95 - Instalar compilador
cd /d "%~dp0"

echo.
echo Baixando compilador i686-elf para Windows...
echo (necessario para gerar o kernel - MinGW sozinho nao basta)
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0instalar-toolchain.ps1"
echo.
pause
