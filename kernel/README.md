# CoreOS 10 Kernel (arquitetura NT)

Kernel do **CoreOS 10** com arquitetura inspirada no **Windows NT 10** — boot direto via MBR, executive em camadas e subsistema Win32.

## Arquitetura NT (estilo Windows 10)

```
┌─────────────────────────────────────────────────────────────┐
│  Subsistema Win32 (csrss/win32k stub) → explorer/desktop    │
└──────────────────────────┬──────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│  NT Executive — ntoskrnl                                    │
│  Ke (kernel) │ Ex (executive) │ Ob (objects) │ Io (drivers) │
│  Mm (memory) │ Ps (process)   │ Se (security)│ Cm (registry)│
└──────────────────────────┬──────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│  HAL — src/hal + arch/x86/hal.c                             │
└──────────────────────────┬──────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│  Drivers KMDF-style — \\Device\\Video0, Keyboard, Mouse       │
└──────────────────────────┬──────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────┐
│  Boot — mbr.S → boot.S @ 0x10000                            │
└─────────────────────────────────────────────────────────────┘
```

### Inicialização (`NtInitializeExecutive`)

1. **Ke** — fase 0 (arquitetura x86)
2. **Mm** — mapa de memória + pool kernel
3. **Ob** — Object Manager (handles)
4. **Se** — token SYSTEM + SRM
5. **Cm** — Registry em memória (`HKLM\...\CurrentVersion`)
6. **Ex** — executive
7. **Ps** — processo System (PID 4)
8. **Io** — I/O Manager + drivers integrados
9. **Ke** fase 1 — GDT (ring 0 + ring 3), IDT, syscalls (`INT 0x2E`)
10. **Win32** — Session Manager → desktop

## Boot

```
BIOS → MBR (setor 0) → kernel @ 0x10000 → menu → NtInitializeExecutive → desktop
```

## Compilar (Windows)

```bat
COMPILAR.bat
```

## Estrutura de diretórios

```
kernel/
├── include/nt/           NTSTATUS, tipos NT
├── include/ntos/         Executive (Ob, Io, Mm, Ps, Se, Cm, Ke, Ex)
├── include/subsys/       Win32 subsystem
├── src/ntos/             Implementação do executive
├── src/subsys/win32/     win32k + Session Manager
├── arch/x86/ke/          GDT, IDT, syscalls
├── arch/x86/drivers/     Drivers integrados (Video, Keyboard, Mouse)
└── src/gui/              Desktop (tema Windows 10)
```

## Roadmap NT

| Fase | Conteúdo |
|------|----------|
| **1.0** ✅ | Executive NT, Ob/Io/Mm/Ps/Se/Cm, Win32 stub, GDT/IDT |
| **1.1** | Scheduler preemptivo, threads reais |
| **1.2** | Syscalls user-mode (ring 3) |
| **1.3** | VFS + NTFS/FAT stub |
| **1.4** | PnP, mais drivers |
