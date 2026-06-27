# CoreOS 10

Sistema operacional com **arquitetura NT** (estilo Windows 10) e desktop nativo — roda no VirtualBox.

## Inicio rapido (Windows)

1. `kernel\COMPILAR.bat` — compila o kernel e gera `coreos.img` / `coreos.vdi`
2. `kernel\CRIAR-VM-VBOX.bat` — cria VM pronta no VirtualBox
3. Inicie a VM → aguarde 5s ou escolha **Modo Live** → desktop grafico

## Arquitetura

O kernel segue o modelo **Windows NT**:

- **HAL** — abstração de hardware
- **Executive** — Object Manager, I/O Manager, Memory/Process/Security Manager, Registry
- **Drivers** — `\\Device\\Video0`, `\\Device\\KeyboardClass0`, `\\Device\\PointerClass0`
- **Subsistema Win32** — Session Manager inicia o explorer/desktop
- **GDT/IDT** — segmentos kernel + user (ring 3), syscalls via `INT 0x2E`

## O que o sistema tem

- Menu de boot: **Instalar** ou **Modo Live** (timeout 5s)
- Splash "Starting Windows 10..."
- **Desktop** com tema Windows 10
- **Taskbar**, menu Iniciar, janelas e icones
- **Mouse** e teclado via drivers NT

## Estrutura

```
kernel/
  include/ntos/   — headers do executive NT
  src/ntos/       — Ob, Io, Mm, Ps, Se, Cm, Ke, Ex
  src/subsys/     — subsistema Win32
  arch/x86/ke/    — GDT, IDT, syscalls
  arch/x86/drivers/
  COMPILAR.bat
```

Veja `kernel/README.md` para detalhes da arquitetura.
