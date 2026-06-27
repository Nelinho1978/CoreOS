#include "coreos/debug_console.h"
#include "coreos/printk.h"
#include "arch/serial.h"
#include "arch/ports.h"
#include "arch/ata.h"
#include "ntos/ldr.h"
#include "ntos/mm.h"
#include "hal/hal.h"

/* ---- Internal helpers ---- */
static int dbg_stricmp(const char *a, const char *b) {
    if (!a || !b) return -1;
    while (*a && *b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca += 0x20;
        if (cb >= 'A' && cb <= 'Z') cb += 0x20;
        if (ca != cb) return (unsigned char)ca - (unsigned char)cb;
        ++a; ++b;
    }
    return *a ? 1 : (*b ? -1 : 0);
}

static void dbg_trim(char *s) {
    char *end;
    if (!s) return;
    while (*s == ' ' || *s == '\t') ++s;
    end = s;
    while (*end) ++end;
    while (end > s && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) --end;
    *end = '\0';
}

static int dbg_parse(char *line, char **argv) {
    int argc = 0;
    char *p = line;

    if (!line || !argv) return 0;

    while (*p && argc < DBG_MAX_ARGS - 1) {
        while (*p == ' ' || *p == '\t') ++p;
        if (!*p) break;

        if (*p == '"') {
            ++p;
            argv[argc++] = p;
            while (*p && *p != '"') ++p;
            if (*p) *p++ = '\0';
        } else {
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t') ++p;
            if (*p) *p++ = '\0';
        }
    }
    argv[argc] = NULL;
    return argc;
}

static void print_hex_dump(const void *data, uint32_t len, uint32_t base_addr) {
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t i, j;

    for (i = 0; i < len; i += 16) {
        kprintf("%08x  ", base_addr + i);
        for (j = 0; j < 16; ++j) {
            if (i + j < len)
                kprintf("%02x ", bytes[i + j]);
            else
                kputs("   ");
            if (j == 7) kputc(' ');
        }
        kputs(" |");
        for (j = 0; j < 16 && i + j < len; ++j) {
            char c = (char)bytes[i + j];
            kputc((c >= 32 && c < 127) ? c : '.');
        }
        kputs("|\n");
    }
}

/* ---- Command handlers ---- */

static void cmd_help(int argc, char **argv) {
    (void)argc; (void)argv;
    kputs("\n=== CoreOS Debug Console ===\n");
    kputs("  help            - Mostra esta ajuda\n");
    kputs("  cls             - Limpa a tela\n");
    kputs("  mem <addr> [len]- Dump de memoria (hex)\n");
    kputs("  modules         - Lista modulos/DLLs carregados\n");
    kputs("  call <addr>     - Chama funcao no endereco\n");
    kputs("  go              - Sai do console e continua boot\n");
    kputs("  stat            - Estatisticas do sistema\n");
    kputs("  ata init        - Inicializa driver ATA\n");
    kputs("  ata read <lba> [count] - Le setores ATA\n");
    kputs("  reboot          - Reinicia o sistema\n");
    kputs("  ls              - Lista diretorio raiz FAT32\n");
    kputs("  load <file>     - Carrega arquivo do disco\n");
    kputs("\n");
}

static void cmd_cls(int argc, char **argv) {
    (void)argc; (void)argv;
    hal_clear_screen();
    kputs("[Console] Tela limpa\n");
}

static void cmd_mem(int argc, char **argv) {
    uint32_t addr = 0;
    uint32_t len = 256;
    void *ptr;

    if (argc < 2) {
        kputs("Uso: mem <endereco> [tamanho]\n");
        return;
    }

    {
        const char *p = argv[1];
        addr = 0;
        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) p += 2;
        while (*p) {
            char c = *p++;
            addr <<= 4;
            if (c >= '0' && c <= '9') addr |= (uint32_t)(c - '0');
            else if (c >= 'a' && c <= 'f') addr |= (uint32_t)(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') addr |= (uint32_t)(c - 'A' + 10);
            else break;
        }
    }

    if (argc >= 3) {
        const char *p = argv[2];
        len = 0;
        while (*p) {
            char c = *p++;
            if (c >= '0' && c <= '9') len = len * 10 + (uint32_t)(c - '0');
            else break;
        }
    }

    if (len > 1024) len = 1024;
    ptr = (void *)addr;
    kprintf("Dump de memoria em 0x%08x (%u bytes):\n", addr, len);
    print_hex_dump(ptr, len, addr);
}

static void cmd_modules(int argc, char **argv) {
    PLDR_MODULE list;
    uint32_t count;
    uint32_t i;

    (void)argc; (void)argv;

    count = LdrGetModuleCount();
    list = LdrGetModuleList();

    kprintf("Modulos carregados: %u\n", count);
    for (i = 0; i < count; ++i) {
        if (list[i].Flags & 1) { /* LDR_FLAGS_LOADED */
            kprintf("  [%u] %s\n", i, list[i].BaseDllName);
            kprintf("       Base: 0x%08x  Size: 0x%x  Entry: 0x%08x\n",
                    (uint32_t)list[i].DllBase,
                    list[i].SizeOfImage,
                    (uint32_t)list[i].EntryPoint);
        }
    }
}

static void cmd_stat(int argc, char **argv) {
    (void)argc; (void)argv;
    kputs("=== System Status ===\n");
    kprintf("Modulos Ldr: %u\n", LdrGetModuleCount());
    kprintf("Pool slots:  16 x 4096 = 64 KB\n");
    kprintf("Arquitetura: x86 (i386)\n");
    kprintf("HAL: %s\n", hal_arch_name());
    kputs("=====================\n");
}

static void cmd_call(int argc, char **argv) {
    uint32_t addr = 0;
    typedef void (*func_t)(void);
    func_t func;

    if (argc < 2) {
        kputs("Uso: call <endereco>\n");
        return;
    }

    {
        const char *p = argv[1];
        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) p += 2;
        while (*p) {
            char c = *p++;
            addr <<= 4;
            if (c >= '0' && c <= '9') addr |= (uint32_t)(c - '0');
            else if (c >= 'a' && c <= 'f') addr |= (uint32_t)(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') addr |= (uint32_t)(c - 'A' + 10);
            else break;
        }
    }

    kprintf("Chamando funcao em 0x%08x...\n", addr);
    func = (func_t)addr;
    func();
    kputs("Retorno OK\n");
}

static void cmd_ata(int argc, char **argv) {
    if (argc < 2) {
        kputs("Uso: ata init | ata read <lba> [count]\n");
        return;
    }

    if (dbg_stricmp(argv[1], "init") == 0) {
        int ok = ata_init();
        kprintf("ATA init: %s\n", ok ? "OK" : "FALHOU");
        return;
    }

    if (dbg_stricmp(argv[1], "read") == 0 && argc >= 3) {
        uint32_t lba = 0;
        uint8_t count = 1;
        uint8_t buffer[512];
        const char *p = argv[2];
        int i;

        while (*p) {
            char c = *p++;
            if (c >= '0' && c <= '9') lba = lba * 10 + (uint32_t)(c - '0');
            else break;
        }

        if (argc >= 4) {
            const char *q = argv[3];
            count = 0;
            while (*q) {
                char c = *q++;
                if (c >= '0' && c <= '9') count = (uint8_t)(count * 10 + (uint32_t)(c - '0'));
                else break;
            }
            if (count > 1) count = 1;
        }

        kprintf("Lendo setor LBA %u...\n", lba);
        if (ata_read_sectors(lba, count, buffer)) {
            kputs("OK:\n");
            for (i = 0; i < 512; i += 16) {
                int j;
                kprintf("%08x  ", lba * 512 + i);
                for (j = 0; j < 16; ++j) {
                    kprintf("%02x ", buffer[i + j]);
                    if (j == 7) kputc(' ');
                }
                kputs(" |");
                for (j = 0; j < 16; ++j) {
                    char c = (char)buffer[i + j];
                    kputc((c >= 32 && c < 127) ? c : '.');
                }
                kputs("|\n");
            }
        } else {
            kputs("FALHOU\n");
        }
        return;
    }

    kputs("Comando ATA desconhecido\n");
}

static void cmd_go(int argc, char **argv) {
    (void)argc; (void)argv;
    kputs("Retornando ao desktop...\n");
}

static void cmd_reboot(int argc, char **argv) {
    (void)argc; (void)argv;
    kputs("Reiniciando...\n");
    /* Wait for keyboard controller to be ready */
    while (inb(0x64) & 0x02) { __asm__ volatile("pause"); }
    outb(0x64, 0xFE);
    hal_halt();
}

static void cmd_ls(int argc, char **argv) {
    (void)argc; (void)argv;
    kputs("[FAT32] Listando diretorio raiz...\n");
    extern int fat32_list_root(void);
    if (!fat32_list_root()) {
        kputs("FAT32 nao inicializado ou disco ausente\n");
    }
}

static void cmd_load(int argc, char **argv) {
    if (argc < 2) {
        kputs("Uso: load <nome_do_arquivo>\n");
        return;
    }
    kprintf("[FAT32] Carregando arquivo: %s\n", argv[1]);
    extern int fat32_load_file(const char *name, void *buffer, uint32_t *size);
    uint8_t buf[65536];
    uint32_t size = 0;
    if (fat32_load_file(argv[1], buf, &size)) {
        kprintf("Arquivo carregado: %u bytes\n", size);
        print_hex_dump(buf, size > 256 ? 256 : size, 0);
        if (size > 256)
            kputs("... (truncado para 256 bytes)\n");
    } else {
        kputs("Falha ao carregar arquivo\n");
    }
}

/* ---- Command dispatch ---- */
typedef struct {
    const char *name;
    void (*handler)(int argc, char **argv);
} dbg_cmd_t;

static const dbg_cmd_t g_cmds[] = {
    {"help",    cmd_help},
    {"?",       cmd_help},
    {"cls",     cmd_cls},
    {"clear",   cmd_cls},
    {"mem",     cmd_mem},
    {"modules", cmd_modules},
    {"call",    cmd_call},
    {"go",      cmd_go},
    {"stat",    cmd_stat},
    {"ata",     cmd_ata},
    {"ls",      cmd_ls},
    {"dir",     cmd_ls},
    {"load",    cmd_load},
    {"reboot",  cmd_reboot},
    {NULL, NULL}
};

void debug_console_exec(const char *cmd_line) {
    char buf[DBG_LINE_BUF];
    char *argv[DBG_MAX_ARGS];
    int argc, i;

    if (!cmd_line) return;

    /* Copy to local buffer */
    for (i = 0; i < DBG_LINE_BUF - 1 && cmd_line[i]; ++i)
        buf[i] = cmd_line[i];
    buf[i] = '\0';
    dbg_trim(buf);
    if (buf[0] == '\0') return;

    argc = dbg_parse(buf, argv);
    if (argc == 0) return;

    /* Dispatch command */
    for (i = 0; g_cmds[i].name; ++i) {
        if (dbg_stricmp(argv[0], g_cmds[i].name) == 0) {
            g_cmds[i].handler(argc, argv);
            return;
        }
    }

    kprintf("Comando desconhecido: %s\n", argv[0]);
    kputs("Digite 'help' para ver os comandos disponiveis\n");
}

void debug_console_init(void) {
    kputs("[Debug] Console serial interativo inicializado\n");
    kputs("[Debug] Envie comandos via porta serial (115200 8N1)\n");
}

void debug_console_run(void) {
    char line[DBG_LINE_BUF];

    kputs("\n=== CoreOS Debug Console ===\n");
    kputs("Digite 'help' para comandos. 'go' para continuar.\n\n");

    for (;;) {
        kputs("CoreOS> ");
        serial_gets(line, sizeof(line));

        if (line[0] == '\0') continue;

        debug_console_exec(line);

        /* 'go' command exits the console */
        {
            int i;
            for (i = 0; g_cmds[i].name; ++i) {
                if (dbg_stricmp(line, g_cmds[i].name) == 0) {
                    if (dbg_stricmp(line, "go") == 0)
                        return;
                    break;
                }
            }
        }
    }
}
