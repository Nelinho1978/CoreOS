#include "pe.h"
#include "coreos/printk.h"
#include "ntos/ldr.h"
#include "string.h"

/* ============================================================
 *  PE Loader — kernel-mode Portable Executable parser
 *
 *  Parses real Windows PE32 .dll/.exe files and loads them
 *  into memory with proper section mapping, import resolution,
 *  export lookup, and base relocations.
 * ============================================================ */

/* Default SectionAlignment if not provided */
#define PE_DEFAULT_ALIGNMENT 4096

/* ---- Helper: string operations (freestanding) ---- */
static int pe_strcmp(const char *a, const char *b) {
    if (!a || !b) return -1;
    while (*a && *b && *a == *b) { ++a; ++b; }
    return (unsigned char)*a - (unsigned char)*b;
}

static int pe_strncmp(const char *a, const char *b, uint32_t n) {
    if (!a || !b) return -1;
    while (n-- > 0 && *a && *b && *a == *b) { ++a; ++b; }
    if (n == (uint32_t)-1) return 0;
    return (unsigned char)*a - (unsigned char)*b;
}

static uint32_t pe_strlen(const char *s) {
    uint32_t len = 0;
    if (!s) return 0;
    while (*s++) ++len;
    return len;
}

static void pe_strcpy(char *dst, const char *src, uint32_t max) {
    uint32_t i;
    if (!dst || !src) return;
    for (i = 0; i < max - 1 && src[i]; ++i)
        dst[i] = src[i];
    dst[i] = '\0';
}

/* ---- Helper: case-insensitive comparison for DLL names ---- */
static int pe_stricmp(const char *a, const char *b) {
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

/* -----------------------------------------------------------
 *  PeRvaToOffset — Convert a Relative Virtual Address (RVA)
 *  to a file offset, given the section headers.
 *  Returns NULL if the RVA is not in any section (e.g. header data).
 * ----------------------------------------------------------- */
void *PeRvaToOffset(uint32_t rva, const IMAGE_SECTION_HEADER *sections,
                    uint16_t num_sections, const void *file_buffer) {
    uint16_t i;

    if (!sections || !file_buffer) return NULL;

    /* Search sections */
    for (i = 0; i < num_sections; ++i) {
        uint32_t va_start = sections[i].VirtualAddress;
        uint32_t va_end   = sections[i].VirtualAddress + sections[i].SizeOfRawData;

        if (rva >= va_start && rva < va_end) {
            uint32_t offset = rva - va_start;
            return (void *)((uint8_t *)file_buffer + sections[i].PointerToRawData + offset);
        }
    }

    /* Not in any section — might be in header area (< SizeOfHeaders) */
    return NULL;
}

/* -----------------------------------------------------------
 *  PeValidateImage — Quick validation of a PE file buffer.
 *  Returns 1 if valid, 0 if invalid.
 * ----------------------------------------------------------- */
int PeValidateImage(const void *file_buffer, uint32_t file_size) {
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_NT_HEADERS32 *nt;

    if (!file_buffer || file_size < sizeof(IMAGE_DOS_HEADER)) {
        return 0;
    }

    dos = (const IMAGE_DOS_HEADER *)file_buffer;

    /* Check MZ signature */
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }

    /* Check e_lfanew within bounds */
    if (dos->e_lfanew < (int32_t)sizeof(IMAGE_DOS_HEADER) ||
        dos->e_lfanew + (int32_t)sizeof(IMAGE_NT_HEADERS32) > (int32_t)file_size) {
        return 0;
    }

    nt = (const IMAGE_NT_HEADERS32 *)((const uint8_t *)file_buffer + dos->e_lfanew);

    /* Check PE signature */
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return 0;
    }

    /* Check machine type (i386 or allow others) */
    if (nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
        return 0;
    }

    /* Check optional header magic */
    if (nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        return 0;
    }

    /* Check size of image doesn't exceed file */
    if (nt->OptionalHeader.SizeOfHeaders > file_size) {
        return 0;
    }

    return 1;
}

/* -----------------------------------------------------------
 *  PeLoadImage — Parse a PE image and prepare it for loading.
 *
 *  file_buffer = raw file contents
 *  file_size   = size of file
 *  load_base   = address where the image is (or will be) loaded
 *
 *  Returns 1 on success, 0 on failure.
 * ----------------------------------------------------------- */
int PeLoadImage(const void *file_buffer, uint32_t file_size,
                void *load_base, uint32_t *size_of_image_out,
                IMAGE_SECTION_HEADER *sections_out,
                uint16_t *num_sections_out,
                IMAGE_NT_HEADERS32 *nt_headers_out) {
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_NT_HEADERS32 *nt;
    const IMAGE_SECTION_HEADER *sections;
    uint16_t i;

    if (!file_buffer || !load_base || !size_of_image_out ||
        !sections_out || !num_sections_out || !nt_headers_out) {
        return 0;
    }

    /* Validate the image */
    if (!PeValidateImage(file_buffer, file_size)) {
        kputs("[PE] Erro: imagem PE invalida\n");
        return 0;
    }

    dos = (const IMAGE_DOS_HEADER *)file_buffer;
    nt  = (const IMAGE_NT_HEADERS32 *)((const uint8_t *)file_buffer + dos->e_lfanew);
    sections = (const IMAGE_SECTION_HEADER *)((const uint8_t *)&nt->OptionalHeader +
                                              nt->FileHeader.SizeOfOptionalHeader);

    *num_sections_out = nt->FileHeader.NumberOfSections;
    *size_of_image_out = nt->OptionalHeader.SizeOfImage;

    /* Copy NT headers for caller use */
    *nt_headers_out = *nt;

    /* ---- Map sections into memory ---- */
    for (i = 0; i < nt->FileHeader.NumberOfSections && i < 64; ++i) {
        uint32_t dest_addr;
        uint32_t raw_size;
        const uint8_t *src;
        uint8_t *dst;

        dest_addr = (uint32_t)load_base + sections[i].VirtualAddress;

        /* Copy raw data from file */
        raw_size = sections[i].SizeOfRawData;
        src = (const uint8_t *)file_buffer + sections[i].PointerToRawData;
        dst = (uint8_t *)dest_addr;

        if (sections[i].PointerToRawData + raw_size > file_size) {
            raw_size = sections[i].SizeOfRawData;
            if (sections[i].PointerToRawData + raw_size > file_size) {
                raw_size = file_size - sections[i].PointerToRawData;
            }
        }

        if (raw_size > 0 && src && dst) {
            uint32_t j;
            for (j = 0; j < raw_size; ++j) {
                dst[j] = src[j];
            }
        }

        /* Zero-fill remainder (virtual size > raw size) */
        {
            uint32_t vsize = sections[i].Misc.VirtualSize;
            if (vsize > raw_size) {
                uint32_t j;
                for (j = raw_size; j < vsize; ++j) {
                    dst[j] = 0;
                }
            }
        }

        /* Save section info for later use */
        sections_out[i] = sections[i];
    }

    kputs("[PE] Imagem carregada: entry=0x");
    {
        char hex[12];
        uint32_t entry = nt->OptionalHeader.AddressOfEntryPoint;
        int j;
        for (j = 7; j >= 0; --j) {
            uint32_t nibble = (entry >> (j * 4)) & 0x0F;
            hex[7 - j] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
        }
        hex[8] = '\0';
        kputs(hex);
    }
    kputs(", secoes=");
    {
        char buf[4];
        uint16_t n = nt->FileHeader.NumberOfSections;
        buf[0] = '0' + (char)(n / 10);
        buf[1] = '0' + (char)(n % 10);
        buf[2] = '\0';
        kputs(buf);
    }
    kputs("\n");

    return 1;
}

/* -----------------------------------------------------------
 *  PeProcessRelocations — Apply base relocations (.reloc section)
 *  when the image is loaded at a different base than preferred.
 * ----------------------------------------------------------- */
int PeProcessRelocations(void *image_base,
                         const IMAGE_NT_HEADERS32 *nt_headers,
                         const IMAGE_SECTION_HEADER *sections) {
    const IMAGE_DATA_DIRECTORY *reloc_dir;
    const IMAGE_BASE_RELOCATION *reloc;
    uint32_t delta;
    uint32_t remaining;
    uint32_t i;

    if (!image_base || !nt_headers || !sections) {
        return 0;
    }

    /* No relocations needed if we loaded at preferred base */
    if ((uint32_t)image_base == nt_headers->OptionalHeader.ImageBase) {
        return 1;
    }

    reloc_dir = &nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

    if (reloc_dir->VirtualAddress == 0 || reloc_dir->Size == 0) {
        /* No relocation table — image must be loaded at preferred base */
        kputs("[PE] Aviso: sem .reloc — necessario ImageBase original\n");
        return (uint32_t)image_base == nt_headers->OptionalHeader.ImageBase ? 1 : 0;
    }

    delta = (uint32_t)image_base - nt_headers->OptionalHeader.ImageBase;

    /* Find .reloc section in file */
    reloc = (const IMAGE_BASE_RELOCATION *)PeRvaToOffset(
        reloc_dir->VirtualAddress, sections,
        nt_headers->FileHeader.NumberOfSections, (void *)image_base);

    if (!reloc) {
        kputs("[PE] Erro: .reloc section nao encontrada\n");
        return 0;
    }

    remaining = reloc_dir->Size;

    while (remaining >= sizeof(IMAGE_BASE_RELOCATION)) {
        uint32_t page_rva = reloc->VirtualAddress;
        uint32_t block_size = reloc->SizeOfBlock;
        uint32_t num_entries;
        uint32_t j;

        if (block_size < sizeof(IMAGE_BASE_RELOCATION)) {
            break;
        }

        num_entries = (block_size - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t);

        /* Entries start right after the header */
        const uint16_t *entries = (const uint16_t *)(reloc + 1);

        for (j = 0; j < num_entries; ++j) {
            uint16_t entry = entries[j];
            uint16_t type = (uint16_t)(entry >> 12);
            uint16_t offset = (uint16_t)(entry & 0x0FFF);

            if (type == IMAGE_REL_BASED_ABSOLUTE) {
                /* Padding/no-op */
                continue;
            }

            if (type == IMAGE_REL_BASED_HIGHLOW) {
                uint32_t *patch_addr = (uint32_t *)((uint8_t *)image_base + page_rva + offset);
                *patch_addr += delta;
            }
            /* For IMAGE_REL_BASED_DIR64 (64-bit), we would handle here */
        }

        /* Advance to next relocation block */
        remaining -= block_size;
        reloc = (const IMAGE_BASE_RELOCATION *)((const uint8_t *)reloc + block_size);
    }

    return 1;
}

/* -----------------------------------------------------------
 *  PeBuildImportTable — Walk the import table (.idata) and
 *  resolve each imported function address in the IAT.
 *
 *  For each import descriptor, we look up the DLL name,
 *  find each imported function by name or ordinal, and
 *  store its address in the IAT (FirstThunk).
 * ----------------------------------------------------------- */
int PeBuildImportTable(void *image_base, PLDR_MODULE module_list,
                       const IMAGE_NT_HEADERS32 *nt_headers,
                       const IMAGE_SECTION_HEADER *sections) {
    const IMAGE_DATA_DIRECTORY *import_dir;
    const IMAGE_IMPORT_DESCRIPTOR *desc;
    uint16_t num_sections;

    if (!image_base || !nt_headers || !sections) return 0;

    import_dir = &nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    num_sections = nt_headers->FileHeader.NumberOfSections;

    if (import_dir->VirtualAddress == 0 || import_dir->Size == 0) {
        return 1; /* No imports — that's OK */
    }

    /* Find import table in loaded image */
    desc = (const IMAGE_IMPORT_DESCRIPTOR *)PeRvaToOffset(
        import_dir->VirtualAddress, sections, num_sections, image_base);

    if (!desc) {
        /* Try via raw file offset — the IAT might be patched at runtime */
        /* Walk from virtual address directly */
        desc = (const IMAGE_IMPORT_DESCRIPTOR *)((uint8_t *)image_base +
                                                  import_dir->VirtualAddress);
    }

    while (desc->DUMMYUNIONNAME.OriginalFirstThunk != 0 ||
           desc->FirstThunk != 0) {
        const char *dll_name;
        const IMAGE_THUNK_DATA32 *orig_thunk;
        IMAGE_THUNK_DATA32 *iat_thunk;
        uint32_t image_base_addr = (uint32_t)image_base;

        /* Get DLL name */
        if (desc->Name) {
            dll_name = (const char *)((uint8_t *)image_base + desc->Name);
        } else {
            dll_name = "";
        }

        /* INT (Import Name Table) — original thunks */
        if (desc->DUMMYUNIONNAME.OriginalFirstThunk) {
            orig_thunk = (const IMAGE_THUNK_DATA32 *)
                ((uint8_t *)image_base + desc->DUMMYUNIONNAME.OriginalFirstThunk);
        } else {
            /* Fallback to IAT */
            orig_thunk = (const IMAGE_THUNK_DATA32 *)
                ((uint8_t *)image_base + desc->FirstThunk);
        }

        /* IAT — will be patched with resolved addresses */
        iat_thunk = (IMAGE_THUNK_DATA32 *)
            ((uint8_t *)image_base + desc->FirstThunk);

        /* Walk each thunk entry */
        while (orig_thunk->u1.AddressOfData != 0) {
            void *func_addr = NULL;
            const char *func_name = NULL;
            uint32_t ordinal = 0;

            if (IMAGE_SNAP_BY_ORDINAL32(orig_thunk->u1.Ordinal)) {
                /* Import by ordinal */
                ordinal = IMAGE_ORDINAL32(orig_thunk->u1.Ordinal);
            } else {
                /* Import by name */
                const IMAGE_IMPORT_BY_NAME *import_name =
                    (const IMAGE_IMPORT_BY_NAME *)
                        ((uint8_t *)image_base + orig_thunk->u1.AddressOfData);
                func_name = (const char *)import_name->Name;
                ordinal = import_name->Hint;
            }

            /* Resolve the import using the Ldr module */
            func_addr = LdrResolveImport(dll_name, func_name, ordinal);

            if (func_addr) {
                /* Patch IAT with resolved address */
                iat_thunk->u1.Function = (uint32_t)func_addr;
            } else {
                /* Try to find stub in our built-in stubs */
                kputs("[PE] Import nao resolvido: ");
                kputs(dll_name);
                kputs(".");
                if (func_name) {
                    kputs(func_name);
                } else {
                    /* Print ordinal */
                    char buf[12];
                    uint32_t ord = ordinal;
                    int j;
                    buf[0] = '#';
                    for (j = 1; j < 10; ++j) buf[j] = '0';
                    buf[10] = '\0';
                    j = 9;
                    while (ord > 0 && j > 0) {
                        buf[j--] = '0' + (ord % 10);
                        ord /= 10;
                    }
                    kputs(buf);
                }
                kputs("\n");
                /* Leave address as 0 — will crash if called, but visible */
                iat_thunk->u1.Function = 0;
            }

            ++orig_thunk;
            ++iat_thunk;
        }

        ++desc;
    }

    return 1;
}

/* -----------------------------------------------------------
 *  PeFindExport — Locate an exported function in a loaded PE
 *  image by name or ordinal.
 * ----------------------------------------------------------- */
void *PeFindExport(const void *image_base,
                   const IMAGE_NT_HEADERS32 *nt_headers,
                   const IMAGE_SECTION_HEADER *sections,
                   const char *name, uint16_t ordinal) {
    const IMAGE_DATA_DIRECTORY *export_dir;
    const IMAGE_EXPORT_DIRECTORY *exp;
    uint32_t *funcs;
    uint32_t *names;
    uint16_t *ordinals;
    uint32_t i;

    if (!image_base || !nt_headers) return NULL;

    export_dir = &nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

    if (export_dir->VirtualAddress == 0 || export_dir->Size == 0) {
        return NULL; /* No exports */
    }

    exp = (const IMAGE_EXPORT_DIRECTORY *)
        ((uint8_t *)image_base + export_dir->VirtualAddress);
    funcs = (uint32_t *)((uint8_t *)image_base + exp->AddressOfFunctions);

    /* Try ordinal lookup first */
    if (ordinal > 0 && ordinal >= exp->Base &&
        ordinal - exp->Base < exp->NumberOfFunctions) {
        uint32_t func_rva = funcs[ordinal - exp->Base];
        if (func_rva != 0) {
            return (void *)((uint8_t *)image_base + func_rva);
        }
    }

    /* Try name lookup */
    if (name && *name) {
        names = (uint32_t *)((uint8_t *)image_base + exp->AddressOfNames);
        ordinals = (uint16_t *)((uint8_t *)image_base + exp->AddressOfNameOrdinals);

        for (i = 0; i < exp->NumberOfNames; ++i) {
            const char *export_name = (const char *)((uint8_t *)image_base + names[i]);
            if (export_name && pe_strcmp(export_name, name) == 0) {
                uint32_t idx = ordinals[i];
                if (idx < exp->NumberOfFunctions) {
                    uint32_t func_rva = funcs[idx];
                    if (func_rva != 0) {
                        return (void *)((uint8_t *)image_base + func_rva);
                    }
                }
            }
        }
    }

    return NULL; /* Not found */
}
