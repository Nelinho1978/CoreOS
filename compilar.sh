#!/bin/bash
# CoreOS 10 - Script de Compilação
# Uso: bash compilar.sh [disk|run|clean|iso]
# Se nenhum argumento for passado, compila o kernel e gera a imagem de disco

set -e

KERNEL_DIR="kernel"
ACTION="${1:-disk}"

echo "============================================"
echo "  CoreOS 10 - Compilação"
echo "  Ação: $ACTION"
echo "  $(date)"
echo "============================================"
echo ""

case "$ACTION" in
    disk|all)
        echo "[1/2] Compilando kernel..."
        echo ""
        cd "$KERNEL_DIR"
        make disk
        cd ..
        echo ""
        echo "[2/2] Verificando artefatos..."
        ls -lh kernel/build/x86/coreos-kernel.flat 2>/dev/null || echo "   (flat: arquivo nao encontrado)"
        ls -lh kernel/build/x86/coreos.img 2>/dev/null || echo "   (img: arquivo nao encontrado)"
        echo ""
        echo "✅ Compilação concluída com sucesso!"
        echo "   Kernel: kernel/build/x86/coreos-kernel.flat"
        echo "   Disco:  kernel/build/x86/coreos.img"
        ;;
    run)
        echo "▶️  Iniciando CoreOS 10 no QEMU..."
        cd "$KERNEL_DIR"
        make run
        cd ..
        ;;
    clean)
        echo "🧹 Limpando arquivos de build..."
        cd "$KERNEL_DIR"
        make clean
        cd ..
        echo "✅ Limpeza concluída."
        ;;
    iso)
        echo "💿 Gerando imagem ISO..."
        cd "$KERNEL_DIR"
        make iso
        cd ..
        echo "✅ ISO gerada em kernel/build/x86/coreos.iso"
        ;;
    help)
        echo "Comandos:"
        echo "  bash compilar.sh          = Compilar kernel + imagem de disco"
        echo "  bash compilar.sh disk     = Compilar kernel + imagem de disco"
        echo "  bash compilar.sh run      = Executar no QEMU"
        echo "  bash compilar.sh clean    = Limpar build"
        echo "  bash compilar.sh iso      = Gerar ISO"
        echo "  bash compilar.sh help     = Mostrar esta ajuda"
        echo ""
        echo "Windows:"
        echo "  kernel\\COMPILAR.bat"
        ;;
    *)
        echo "❌ Ação desconhecida: $ACTION"
        echo "Use: bash compilar.sh help"
        exit 1
        ;;
esac
