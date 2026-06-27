# CoreOS 95 no VirtualBox (Windows)

## ATENCAO — erro VERR_NOT_SUPPORTED

Se aparecer **"Falha ao abrir o arquivo de imagem de disco ... coreos.iso"**:

| Voce fez isto | O que fazer |
|---------------|-------------|
| Anexou `.iso` como **disco rigido** | **ERRADO** — ISO nao e disco VDI/VHD |
| Quer disco rigido | Use **`coreos.img`** ou rode **`CRIAR-VDI.bat`** |
| Quer usar a ISO | Anexe na **unidade CD (optica)**, nao no HD |

---

## Forma mais facil (recomendada)

1. Rode **`COMPILAR.bat`**
2. VirtualBox → VM → **Configuracoes** → **Armazenamento**
3. **Controlador IDE** → **Adicionar disco rigido** → arquivo existente
4. Filtro: **Todos os arquivos (*.*)**
5. Selecione: `C:\open-core-os\kernel\build\x86\coreos.img`
6. **Manter formato existente** (nao converter)
7. **Sistema** → **EFI: DESLIGADO**
8. Iniciar

**Opcional:** rode **`CRIAR-VDI.bat`** para gerar `coreos.vdi` (VirtualBox aceita sem erro).

---

## Usar ISO (CD virtual)

A ISO **so funciona na unidade optica (CD)**, nunca como disco rigido.

1. Rode **`GERAR-ISO.bat`** (regenera `coreos.iso` corrigida)
2. **Armazenamento** → **Controlador IDE** → clique no icone **CD vazio**
3. A direita: **Escolher um arquivo de disco** → `coreos.iso`
4. **Sistema** → boot **Optico** primeiro, **EFI desligado**

---

## Configuração da VM

- **Tipo:** Other / Unknown (32-bit)
- **EFI:** desligado (boot BIOS/MBR)
- **Controlador:** IDE (não SATA, para o `.img`)
- **Memória:** 32–64 MB

---

## Tela preta ou não boota

1. Confirme que **EFI está desligado**
2. Recompile: `COMPILAR.bat`
3. Prefira o **`.img` como disco IDE** em vez da ISO
4. Verifique se `coreos.img` tem ~1,4 MB (1 474 560 bytes)

---

## Compilar (resumo)

```
1. instalar-toolchain.bat  (se ainda nao fez)
2. COMPILAR.bat            -> coreos.img
3. GERAR-ISO.bat           -> coreos.img + coreos.iso
4. VirtualBox: anexar .img (IDE) ou .iso (CD)
```
