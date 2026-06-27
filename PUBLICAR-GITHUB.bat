@echo off
setlocal EnableExtensions
title Publicar CoreOS no GitHub
cd /d "%~dp0"

where gh >nul 2>&1
if errorlevel 1 (
    echo [ERRO] GitHub CLI nao encontrado. Instale com:
    echo   winget install GitHub.cli
    pause
    exit /b 1
)

gh auth status >nul 2>&1
if errorlevel 1 (
    echo.
    echo === Login no GitHub ===
    echo Siga as instrucoes na tela ou no navegador.
    echo.
    gh auth login -h github.com -p https -w
    if errorlevel 1 (
        echo [ERRO] Login cancelado ou falhou.
        pause
        exit /b 1
    )
)

echo.
echo === Criando repositorio CoreOS e enviando codigo ===
gh repo view CoreOS >nul 2>&1
if errorlevel 1 (
    gh repo create CoreOS --public --source=. --remote=origin --description "Sistema operacional CoreOS com kernel NT, desktop grafico, USB e aceleracao grafica." --push
) else (
    for /f "delims=" %%U in ('gh api user -q .login 2^>nul') do set "GHUSER=%%U"
    git remote get-url origin >nul 2>&1
    if errorlevel 1 git remote add origin https://github.com/%GHUSER%/CoreOS.git
    git push -u origin main
)

if errorlevel 1 (
    echo.
    echo [ERRO] Falha ao publicar. Verifique se o repositorio CoreOS ja existe
    echo e se voce tem permissao na conta GitHub logada.
    pause
    exit /b 1
)

for /f "delims=" %%U in ('gh api user -q .login 2^>nul') do set "GHUSER=%%U"
echo.
echo [OK] Repositorio publicado:
echo   https://github.com/%GHUSER%/CoreOS
echo.
pause
exit /b 0
