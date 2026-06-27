#Requires -Version 5.1
<#
  Baixa toolchain i686-elf para compilar o kernel no Windows.
  Tamanho aproximado: 200-900 MB conforme a versao.
#>

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
$ToolsRoot = Join-Path $Root "tools"
$MarkerFile = Join-Path $ToolsRoot "toolchain-ready.txt"

function Ensure-Dir($p) {
    if (-not (Test-Path $p)) { New-Item -ItemType Directory -Path $p -Force | Out-Null }
}

function Find-Gcc {
    if (-not (Test-Path $ToolsRoot)) { return $null }
    return Get-ChildItem -Path $ToolsRoot -Recurse -Filter "i686-elf-gcc.exe" -ErrorAction SilentlyContinue |
        Select-Object -First 1
}

function Write-ToolchainMarker($gccPath) {
    $binDir = Split-Path $gccPath -Parent
    Ensure-Dir $ToolsRoot
    Set-Content -Path $MarkerFile -Value $binDir -Encoding ASCII
    Write-Host "Toolchain pronta em: $binDir" -ForegroundColor Green
}

$existing = Find-Gcc
if ($existing) {
    Write-ToolchainMarker $existing.FullName
    exit 0
}

# Versao 7.1.0 = testada e menor. 15.2.0 = mais recente (arquivo grande).
$Downloads = @(
    @{
        Name = "GCC 7.1.0 (recomendado, ~200 MB)"
        Url  = "https://github.com/lordmilko/i686-elf-tools/releases/download/7.1.0/i686-elf-tools-windows.zip"
        Zip  = Join-Path $ToolsRoot "i686-elf-7.1.0.zip"
    },
    @{
        Name = "GCC 15.2.0 (recente, ~900 MB)"
        Url  = "https://github.com/lordmilko/i686-elf-tools/releases/download/15.2.0/i686-elf-tools-windows.zip"
        Zip  = Join-Path $ToolsRoot "i686-elf-15.2.0.zip"
    }
)

Ensure-Dir $ToolsRoot

function Download-File($url, $dest) {
    Write-Host "Baixando..." -ForegroundColor Cyan
    Write-Host "  $url"

    # 1) curl.exe (vem no Windows 10/11) - melhor para arquivos grandes
    $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
    if ($curl) {
        Write-Host "  Usando curl.exe (pode demorar varios minutos)..." -ForegroundColor DarkGray
        & curl.exe -L --retry 5 --retry-delay 3 --progress-bar -o $dest $url
        if ($LASTEXITCODE -eq 0 -and (Test-Path $dest) -and (Get-Item $dest).Length -gt 1MB) {
            return $true
        }
        Remove-Item $dest -Force -ErrorAction SilentlyContinue
    }

    # 2) BITS (retomavel)
    try {
        Write-Host "  Usando BITS..." -ForegroundColor DarkGray
        Import-Module BitsTransfer -ErrorAction Stop
        Start-BitsTransfer -Source $url -Destination $dest -Description "CoreOS i686-elf"
        if ((Test-Path $dest) -and (Get-Item $dest).Length -gt 1MB) { return $true }
    }
    catch {
        Remove-Item $dest -Force -ErrorAction SilentlyContinue
    }

    # 3) Invoke-WebRequest (ultimo recurso)
    try {
        Write-Host "  Usando Invoke-WebRequest..." -ForegroundColor DarkGray
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $url -OutFile $dest -UseBasicParsing -TimeoutSec 3600
        if ((Test-Path $dest) -and (Get-Item $dest).Length -gt 1MB) { return $true }
    }
    catch {
        Remove-Item $dest -Force -ErrorAction SilentlyContinue
    }

    return $false
}

$ok = $false
foreach ($item in $Downloads) {
    Write-Host ""
    Write-Host "Tentando: $($item.Name)" -ForegroundColor White

    if (Download-File $item.Url $item.Zip) {
        $sizeMb = [math]::Round((Get-Item $item.Zip).Length / 1MB, 1)
        Write-Host "Download OK ($sizeMb MB). Extraindo..." -ForegroundColor Green

        $extractTo = Join-Path $ToolsRoot "extract-tmp"
        if (Test-Path $extractTo) { Remove-Item -Recurse -Force $extractTo }
        Ensure-Dir $extractTo

        Expand-Archive -Path $item.Zip -DestinationPath $extractTo -Force

        $gcc = Get-ChildItem -Path $extractTo -Recurse -Filter "i686-elf-gcc.exe" -ErrorAction SilentlyContinue |
            Select-Object -First 1

        if (-not $gcc) {
            Write-Host "ERRO: i686-elf-gcc.exe nao encontrado no zip." -ForegroundColor Red
            continue
        }

        # Copiar pasta raiz da toolchain (pai de bin/) para tools/i686-elf
        $toolchainRoot = $gcc.Directory.Parent.FullName
        $destDir = Join-Path $ToolsRoot "i686-elf"
        if (Test-Path $destDir) { Remove-Item -Recurse -Force $destDir }
        Copy-Item -Path $toolchainRoot -Destination $destDir -Recurse -Force

        Remove-Item $extractTo -Recurse -Force -ErrorAction SilentlyContinue
        Remove-Item $item.Zip -Force -ErrorAction SilentlyContinue

        $finalGcc = Join-Path $destDir "bin\i686-elf-gcc.exe"
        if (-not (Test-Path $finalGcc)) {
            # zip pode ter estrutura bin direto na raiz
            $finalGcc = $gcc.FullName
        }

        Write-ToolchainMarker ((Find-Gcc).FullName)
        $ok = $true
        break
    }

    Write-Host "Falha neste espelho." -ForegroundColor Yellow
}

if (-not $ok) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "  Nao foi possivel baixar automaticamente" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    Write-Host ""
    Write-Host "INSTALACAO MANUAL (mais confiavel):" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "1. Abra no navegador (Chrome/Edge):" -ForegroundColor White
    Write-Host "   https://github.com/lordmilko/i686-elf-tools/releases/tag/7.1.0"
    Write-Host ""
    Write-Host "2. Baixe: i686-elf-tools-windows.zip"
    Write-Host ""
    Write-Host "3. Extraia o ZIP inteiro em:" -ForegroundColor White
    Write-Host "   $ToolsRoot\i686-elf"
    Write-Host ""
    Write-Host "4. Deve existir o arquivo:" -ForegroundColor White
    Write-Host "   $ToolsRoot\i686-elf\bin\i686-elf-gcc.exe"
    Write-Host ""
    Write-Host "5. Rode COMPILAR.bat novamente."
    Write-Host ""
    Write-Host "Veja tambem: INSTALAR-MANUAL.txt"
    Write-Host ""
    exit 1
}

exit 0
