#Requires -Version 5.1
<#
  Redireciona para build.sh no MSYS2 (usa i686-elf, NAO MinGW).
  NAO compila com MinGW - isso gera erro de link PE/ELF.
#>

param(
    [ValidateSet("disk", "iso", "help")]
    [string]$Target = "disk"
)

$Msys = "C:\msys64\msys2_shell.cmd"
if (-not (Test-Path $Msys)) {
    Write-Host "ERRO: MSYS2 nao encontrado em C:\msys64" -ForegroundColor Red
    exit 1
}

if ($Target -eq "help") {
    Write-Host @"

Use os arquivos .BAT (mais facil):
  COMPILAR.bat     -> gera coreos.img
  GERAR-ISO.bat    -> gera coreos.img + coreos.iso

Requisitos:
  - MSYS2 + nasm (pacman -S nasm)
  - Toolchain i686-elf (instalar-toolchain.bat ou INSTALAR-MANUAL.txt)

"@
    exit 0
}

# Verificar toolchain
$gcc = Get-ChildItem -Path (Join-Path $PSScriptRoot "tools") -Recurse -Filter "i686-elf-gcc.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $gcc) {
    Write-Host "ERRO: i686-elf-gcc nao encontrado em tools\" -ForegroundColor Red
    Write-Host "Rode: instalar-toolchain.bat" -ForegroundColor Yellow
    Write-Host "Ou leia: INSTALAR-MANUAL.txt" -ForegroundColor Yellow
    exit 1
}

& $Msys -mingw32 -defterm -no-start -here -c "bash ./build.sh $Target"
exit $LASTEXITCODE
