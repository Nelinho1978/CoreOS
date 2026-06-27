#include "ntos/ldr.h"
#include "coreos/printk.h"
#include "pe.h"
#include "ntos/mm.h"

/* ============================================================
 *  Ldr Module — Windows NT-style Dynamic Link Library Loader
 *
 *  Manages the list of loaded PE modules and provides
 *  LdrLoadDll, LdrGetProcedureAddress, LdrUnloadDll,
 *  and LdrResolveImport for the PE loader.
 * ============================================================ */

/* Global module list */
static LDR_MODULE g_module_list[LDR_MAX_MODULES];
static uint32_t g_module_count;

/* Forward declarations for stub resolution */
extern void *Win32StubResolve(const char *dll_name, const char *func_name);

/* ---- Internal: find a free module slot ---- */
static PLDR_MODULE ldr_find_free_slot(void) {
    uint32_t i;
    for (i = 0; i < LDR_MAX_MODULES; ++i) {
        if (!(g_module_list[i].Flags & LDR_FLAGS_LOADED)) {
            return &g_module_list[i];
        }
    }
    return NULL;
}

/* ---- Internal: simple string operations ---- */
static int ldr_stricmp(const char *a, const char *b) {
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

static int ldr_strcmp(const char *a, const char *b) {
    if (!a || !b) return -1;
    while (*a && *b && *a == *b) { ++a; ++b; }
    return (unsigned char)*a - (unsigned char)*b;
}

static uint32_t ldr_strlen(const char *s) {
    uint32_t len = 0;
    if (!s) return 0;
    while (s[len]) ++len;
    return len;
}

static void ldr_strcpy(char *dst, const char *src, uint32_t max) {
    uint32_t i;
    if (!dst || !src) return;
    for (i = 0; i < max - 1 && src[i]; ++i)
        dst[i] = src[i];
    dst[i] = '\0';
}

/* ---- Internal: extract base DLL name from path ---- */
static const char *ldr_base_name(const char *path) {
    const char *last = path;
    if (!path) return "";
    while (*path) {
        if (*path == '/' || *path == '\\') last = path + 1;
        ++path;
    }
    return last;
}

/* ---- FAT32 file loading ---- */
extern int fat32_load_file(const char *name, void *buffer, uint32_t *size);

/* -----------------------------------------------------------
 *  LdrInitSystem — Initialize the module loader.
 * ----------------------------------------------------------- */
void LdrInitSystem(void) {
    uint32_t i;
    for (i = 0; i < LDR_MAX_MODULES; ++i) {
        g_module_list[i].Flags = 0;
        g_module_list[i].DllBase = NULL;
        g_module_list[i].Next = NULL;
        g_module_list[i].Prev = NULL;
    }
    g_module_count = 0;
    kputs("[Ldr] Module Loader inicializado\n");
}

/* -----------------------------------------------------------
 *  LdrAllocateImageMemory — Allocate memory for a PE image.
 *  Uses MmAllocatePool for now (limited to 64KB).
 *  Future: use proper virtual memory allocator.
 * ----------------------------------------------------------- */
void *LdrAllocateImageMemory(uint32_t size) {
    /* We use MmAllocatePool. For images > 4KB we might need multiple
     * pool pages. For now, we allocate the largest available slot.
     * In a real implementation, this would use MmMapViewOfSection
     * or virtual memory allocation. */
    extern void *MmAllocatePool(uint32_t bytes);

    kputs("[Ldr] Alocando memoria para imagem: ");
    {
        char buf[16];
        uint32_t sz = size;
        int j = 0;
        buf[0] = '0';
        buf[1] = 'x';
        for (j = 0; j < 8; ++j) {
            uint32_t nibble = (sz >> (28 - j * 4)) & 0x0F;
            buf[2 + j] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
        }
        buf[10] = '\0';
        kputs(buf);
    }
    kputs(" bytes\n");

    /* For kernel-mode loading, we allocate from the pool.
     * This works for small DLLs. For larger images we'd need
     * proper VM allocation via MmMapViewOfSection. */
    return MmAllocatePool(size);
}

/* -----------------------------------------------------------
 *  LdrFreeImageMemory — Free memory used by a PE image.
 * ----------------------------------------------------------- */
void LdrFreeImageMemory(void *base, uint32_t size) {
    extern void MmFreePool(void *ptr);
    (void)size;
    if (base) {
        MmFreePool(base);
    }
}

/* -----------------------------------------------------------
 *  LdrCallDllEntry — Call DllMain with a given reason.
 *  DllMain signature: BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID)
 * ----------------------------------------------------------- */
NTSTATUS LdrCallDllEntry(void *entry, void *dll_base, uint32_t reason, void *reserved) {
    typedef int (*DLL_MAIN)(void *, uint32_t, void *);
    DLL_MAIN dll_main;
    int result;

    if (!entry) return STATUS_SUCCESS;

    dll_main = (DLL_MAIN)entry;
    result = dll_main(dll_base, reason, reserved);

    return result ? STATUS_SUCCESS : STATUS_ACCESS_DENIED;
}

/* -----------------------------------------------------------
 *  LdrLoadDll — Load a DLL PE file into memory.
 *
 *  Steps:
 *    1. Check if already loaded
 *    2. Read file from disk (via firmware/simple loader)
 *    3. Validate PE header
 *    4. Allocate memory for image
 *    5. Load sections
 *    6. Process relocations
 *    7. Build import table (resolve dependencies)
 *    8. Call DllMain(DLL_PROCESS_ATTACH)
 *    9. Add to module list
 *
 *  NOTE: In the current kernel mode, we load DLLs into kernel
 *  pool memory. A real implementation would load into user-mode
 *  virtual address space.
 * ----------------------------------------------------------- */
void *LdrLoadDll(const char *dll_path, const char *dll_name) {
    const char *name;
    PLDR_MODULE existing;
    PLDR_MODULE mod;
    uint32_t file_size;
    void *file_buffer;

    /* Check if already loaded */
    name = dll_name ? dll_name : ldr_base_name(dll_path);
    existing = LdrFindEntryForName(name);
    if (existing) {
        existing->LoadCount++;
        return existing->DllBase;
    }

    kputs("[Ldr] Carregando DLL: ");
    kputs(name ? name : "(null)");
    kputs("\n");

    /* ---- Stage 1: Check built-in stubs first ---- */
    {
        if (ldr_stricmp(name, "ntdll.dll") == 0) {
            mod = ldr_find_free_slot();
            if (!mod) return NULL;
            mod->DllBase = (void *)0xB0000001;
            mod->ImageBase = 0xB0000000;
            mod->SizeOfImage = 0x10000;
            mod->EntryPoint = NULL;
            mod->LoadCount = 1;
            mod->Flags = LDR_FLAGS_LOADED | LDR_FLAGS_IMAGE_DLL;
            ldr_strcpy(mod->BaseDllName, "ntdll.dll", sizeof(mod->BaseDllName));
            ldr_strcpy(mod->FullDllName, "C:\\Windows\\System32\\ntdll.dll", sizeof(mod->FullDllName));
            mod->Next = NULL; mod->Prev = NULL;
            g_module_count++;
            kputs("[Ldr] NTDLL carregada (stub built-in)\n");
            return mod->DllBase;
        }
        if (ldr_stricmp(name, "kernel32.dll") == 0) {
            mod = ldr_find_free_slot();
            if (!mod) return NULL;
            mod->DllBase = (void *)0xB0001001;
            mod->ImageBase = 0xB0001000;
            mod->SizeOfImage = 0x20000;
            mod->EntryPoint = NULL;
            mod->LoadCount = 1;
            mod->Flags = LDR_FLAGS_LOADED | LDR_FLAGS_IMAGE_DLL;
            ldr_strcpy(mod->BaseDllName, "kernel32.dll", sizeof(mod->BaseDllName));
            ldr_strcpy(mod->FullDllName, "C:\\Windows\\System32\\kernel32.dll", sizeof(mod->FullDllName));
            mod->Next = NULL; mod->Prev = NULL;
            g_module_count++;
            kputs("[Ldr] KERNEL32 carregada (stub built-in)\n");
            return mod->DllBase;
        }
        if (ldr_stricmp(name, "user32.dll") == 0) {
            mod = ldr_find_free_slot();
            if (!mod) return NULL;
            mod->DllBase = (void *)0xB0020001;
            mod->ImageBase = 0xB0020000;
            mod->SizeOfImage = 0x30000;
            mod->EntryPoint = NULL;
            mod->LoadCount = 1;
            mod->Flags = LDR_FLAGS_LOADED | LDR_FLAGS_IMAGE_DLL;
            ldr_strcpy(mod->BaseDllName, "user32.dll", sizeof(mod->BaseDllName));
            ldr_strcpy(mod->FullDllName, "C:\\Windows\\System32\\user32.dll", sizeof(mod->FullDllName));
            mod->Next = NULL; mod->Prev = NULL;
            g_module_count++;
            kputs("[Ldr] USER32 carregada (stub built-in)\n");
            return mod->DllBase;
        }
        if (ldr_stricmp(name, "gdi32.dll") == 0) {
            mod = ldr_find_free_slot();
            if (!mod) return NULL;
            mod->DllBase = (void *)0xB0030001;
            mod->ImageBase = 0xB0030000;
            mod->SizeOfImage = 0x20000;
            mod->EntryPoint = NULL;
            mod->LoadCount = 1;
            mod->Flags = LDR_FLAGS_LOADED | LDR_FLAGS_IMAGE_DLL;
            ldr_strcpy(mod->BaseDllName, "gdi32.dll", sizeof(mod->BaseDllName));
            ldr_strcpy(mod->FullDllName, "C:\\Windows\\System32\\gdi32.dll", sizeof(mod->FullDllName));
            mod->Next = NULL; mod->Prev = NULL;
            g_module_count++;
            kputs("[Ldr] GDI32 carregada (stub built-in)\n");
            return mod->DllBase;
        }
    }

    /* ---- Stage 2: Try to load from disk via FAT32 ---- */
    {
        uint8_t *file_buf = NULL;
        uint32_t file_size = 131072;  /* Max expected DLL size: 128KB */
        void *image_base = NULL;
        int ok = 0;

        kputs("[Ldr] Tentando carregar do disco: ");
        kputs(name);
        kputs("\n");

        /* Allocate file buffer from heap */
        file_buf = MmAllocatePool(file_size);
        if (!file_buf) {
            kputs("[Ldr] Falha ao alocar buffer\n");
            return NULL;
        }

        /* Try to load file — fat32_load_file sets file_size to actual size */
        if (!fat32_load_file(name, file_buf, &file_size)) {
            kputs("[Ldr] Arquivo nao encontrado no disco\n");
            MmFreePool(file_buf);
            return NULL;
        }

        kprintf("[Ldr] Arquivo lido: %u bytes\n", file_size);

        /* Check PE magic */
        if (!PeValidateImage(file_buf, file_size)) {
            kputs("[Ldr] Falha: nao e um PE valido\n");
            MmFreePool(file_buf);
            return NULL;
        }

        /* Parse PE headers to get image size */
        {
            const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)file_buf;
            const IMAGE_NT_HEADERS32 *nt = (const IMAGE_NT_HEADERS32 *)(file_buf + dos->e_lfanew);
            uint32_t size_of_image = nt->OptionalHeader.SizeOfImage;
            IMAGE_SECTION_HEADER sections[64];
            uint16_t num_sections = 0;
            IMAGE_NT_HEADERS32 nt_headers;

            image_base = LdrAllocateImageMemory(size_of_image);
            if (!image_base) {
                kputs("[Ldr] Falha ao alocar memoria\n");
                MmFreePool(file_buf);
                return NULL;
            }

            /* Parse and load PE image */
            if (!PeLoadImage(file_buf, file_size, image_base,
                            &size_of_image, sections, &num_sections, &nt_headers)) {
                kputs("[Ldr] Falha ao carregar PE\n");
                LdrFreeImageMemory(image_base, size_of_image);
                MmFreePool(file_buf);
                return NULL;
            }

            /* Apply relocations */
            PeProcessRelocations(image_base, &nt_headers, sections);

            /* Resolve imports */
            PeBuildImportTable(image_base, g_module_list, &nt_headers, sections);

            /* Register module */
            mod = ldr_find_free_slot();
            if (!mod) {
                LdrFreeImageMemory(image_base, size_of_image);
                MmFreePool(file_buf);
                return NULL;
            }

            mod->DllBase = image_base;
            mod->ImageBase = nt_headers.OptionalHeader.ImageBase;
            mod->SizeOfImage = size_of_image;
            mod->EntryPoint = (void *)((uint8_t *)image_base +
                                        nt_headers.OptionalHeader.AddressOfEntryPoint);
            mod->LoadCount = 1;
            mod->Flags = LDR_FLAGS_LOADED | LDR_FLAGS_IMAGE_DLL;
            ldr_strcpy(mod->BaseDllName, name, sizeof(mod->BaseDllName));
            {
                uint32_t j;
                for (j = 0; j < 127 && name[j]; ++j)
                    mod->FullDllName[j] = name[j];
                mod->FullDllName[127] = '\0';
            }
            mod->Next = NULL; mod->Prev = NULL;
            g_module_count++;

            /* Call DllMain(DLL_PROCESS_ATTACH) if entry point exists */
            if (mod->EntryPoint) {
                LdrCallDllEntry(mod->EntryPoint, image_base, 1, NULL);
            }

            ok = 1;
        }

        MmFreePool(file_buf);

        if (ok) {
            kputs("[Ldr] DLL carregada do disco\n");
            return image_base;
        }
    }

    /* Unknown DLL — not found anywhere */
    kputs("[Ldr] DLL nao encontrada: ");
    kputs(name);
    kputs("\n");
    return NULL;
}

/* -----------------------------------------------------------
 *  LdrGetProcedureAddress — Look up an exported function
 *  from a loaded module by name.
 * ----------------------------------------------------------- */
void *LdrGetProcedureAddress(void *module_handle, const char *function_name) {
    PLDR_MODULE mod;
    uint32_t i;

    if (!module_handle || !function_name) return NULL;

    /* Find module by handle (base address) */
    mod = NULL;
    for (i = 0; i < LDR_MAX_MODULES; ++i) {
        if ((g_module_list[i].Flags & LDR_FLAGS_LOADED) &&
            g_module_list[i].DllBase == module_handle) {
            mod = &g_module_list[i];
            break;
        }
    }

    if (!mod) return NULL;

    /* Try to resolve from our stub functions */
    {
        extern void *Win32StubResolve(const char *dll_name, const char *func_name);
        void *stub_addr = Win32StubResolve(mod->BaseDllName, function_name);
        if (stub_addr) {
            kputs("[Ldr] GetProcAddress: ");
            kputs(function_name);
            kputs(" -> STUB\n");
            return stub_addr;
        }
    }

    /* If the module was loaded from a real PE file, search exports */
    kputs("[Ldr] GetProcAddress: ");
    kputs(function_name);
    kputs(" -> nao encontrada\n");

    /* Return the function name pointer as a signal for debugging */
    return (void *)function_name;
}

/* -----------------------------------------------------------
 *  LdrUnloadDll — Unload a DLL from memory.
 * ----------------------------------------------------------- */
NTSTATUS LdrUnloadDll(void *module_handle) {
    PLDR_MODULE mod;
    uint32_t i;

    if (!module_handle) return STATUS_INVALID_PARAMETER;

    for (i = 0; i < LDR_MAX_MODULES; ++i) {
        if ((g_module_list[i].Flags & LDR_FLAGS_LOADED) &&
            g_module_list[i].DllBase == module_handle) {
            mod = &g_module_list[i];

            if (mod->LoadCount > 1) {
                mod->LoadCount--;
                return STATUS_SUCCESS;
            }

            /* Call DllMain(DLL_PROCESS_DETACH) */
            if (mod->EntryPoint) {
                LdrCallDllEntry(mod->EntryPoint, mod->DllBase, 0, NULL);
            }

            /* Free memory */
            if (mod->SizeOfImage > 0 && mod->DllBase) {
                LdrFreeImageMemory(mod->DllBase, mod->SizeOfImage);
            }

            /* Clear slot */
            mod->Flags = 0;
            mod->DllBase = NULL;
            mod->LoadCount = 0;
            g_module_count--;

            kputs("[Ldr] DLL descarregada\n");
            return STATUS_SUCCESS;
        }
    }

    return STATUS_OBJECT_NAME_NOT_FOUND;
}

/* -----------------------------------------------------------
 *  LdrFindEntryForAddress — Find module by code address.
 * ----------------------------------------------------------- */
PLDR_MODULE LdrFindEntryForAddress(const void *address) {
    uint32_t addr = (uint32_t)address;
    uint32_t i;

    for (i = 0; i < LDR_MAX_MODULES; ++i) {
        if (g_module_list[i].Flags & LDR_FLAGS_LOADED) {
            uint32_t base = (uint32_t)g_module_list[i].DllBase;
            if (addr >= base && addr < base + g_module_list[i].SizeOfImage) {
                return &g_module_list[i];
            }
        }
    }

    return NULL;
}

/* -----------------------------------------------------------
 *  LdrFindEntryForName — Find module by DLL name.
 * ----------------------------------------------------------- */
PLDR_MODULE LdrFindEntryForName(const char *dll_name) {
    uint32_t i;

    if (!dll_name) return NULL;

    for (i = 0; i < LDR_MAX_MODULES; ++i) {
        if ((g_module_list[i].Flags & LDR_FLAGS_LOADED) &&
            ldr_stricmp(g_module_list[i].BaseDllName, dll_name) == 0) {
            return &g_module_list[i];
        }
    }

    return NULL;
}

/* -----------------------------------------------------------
 *  LdrGetModuleHandle — Return handle for GetModuleHandle.
 * ----------------------------------------------------------- */
void *LdrGetModuleHandle(const char *module_name) {
    PLDR_MODULE mod;

    if (!module_name) {
        /* Return handle to main executable (kernel itself) */
        if (g_module_count > 0) {
            return g_module_list[0].DllBase;
        }
        return NULL;
    }

    mod = LdrFindEntryForName(module_name);
    return mod ? mod->DllBase : NULL;
}

/* -----------------------------------------------------------
 *  LdrGetModuleCount / LdrGetModuleList — Debug helpers.
 * ----------------------------------------------------------- */
uint32_t LdrGetModuleCount(void) {
    return g_module_count;
}

PLDR_MODULE LdrGetModuleList(void) {
    return g_module_list;
}

/* -----------------------------------------------------------
 *  LdrResolveImport — Called by PeBuildImportTable to resolve
 *  a function import from any loaded module.
 * ----------------------------------------------------------- */
void *LdrResolveImport(const char *dll_name, const char *func_name, uint32_t ordinal) {
    PLDR_MODULE mod;
    uint32_t i;

    if (!dll_name) return NULL;

    /* Find the DLL in our loaded modules */
    mod = LdrFindEntryForName(dll_name);
    if (!mod) {
        /* Try to auto-load the DLL */
        void *handle = LdrLoadDll(NULL, dll_name);
        if (!handle) {
            /* Check stub table */
            void *stub = Win32StubResolve(dll_name, func_name);
            if (stub) return stub;
            return NULL;
        }
        mod = LdrFindEntryForName(dll_name);
        if (!mod) return NULL;
    }

    /* If the module has a real PE image, search exports */
    if (mod->DllBase && (uint32_t)mod->DllBase > 0x10000000) {
        /* Real PE image — would need to search exports */
        /* For now, fall through to stub resolution */
    }

    /* Try stub resolution */
    {
        void *stub = Win32StubResolve(dll_name, func_name);
        if (stub) return stub;
    }

    /* Last resort: try direct function by ordinal */
    /* For stubs, we return a placeholder if name is provided */
    if (func_name) {
        /* Return the name pointer as debug signal */
        return (void *)func_name;
    }

    return NULL;
}
