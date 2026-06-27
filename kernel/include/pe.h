#ifndef COREOS_PE_H
#define COREOS_PE_H

#include "coreos/types.h"
#include "nt/ntdef.h"

/* ============================================================
 *  PE32 (Portable Executable) Format Headers
 *  Compatible with Windows winnt.h definitions
 * ============================================================ */

/* --- DOS Header (starts at offset 0) --- */
typedef struct _IMAGE_DOS_HEADER {
    uint16_t e_magic;       /* MZ magic: 0x5A4D */
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    int32_t  e_lfanew;      /* RVA to IMAGE_NT_HEADERS */
} __attribute__((packed)) IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

#define IMAGE_DOS_SIGNATURE  0x5A4D  /* "MZ" */

/* --- File Header (part of NT_HEADERS) --- */
typedef struct _IMAGE_FILE_HEADER {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} __attribute__((packed)) IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

#define IMAGE_FILE_MACHINE_I386       0x014C
#define IMAGE_FILE_MACHINE_AMD64      0x8664
#define IMAGE_FILE_MACHINE_ARM        0x01C4
#define IMAGE_FILE_MACHINE_THUMB      0x01C2

#define IMAGE_FILE_RELOCS_STRIPPED    0x0001
#define IMAGE_FILE_EXECUTABLE_IMAGE   0x0002
#define IMAGE_FILE_LINE_NUMS_STRIPPED 0x0004
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED 0x0008
#define IMAGE_FILE_LARGE_ADDRESS_AWARE 0x0020
#define IMAGE_FILE_32BIT_MACHINE      0x0100
#define IMAGE_FILE_DLL                0x2000
#define IMAGE_FILE_SYSTEM             0x1000

/* --- Optional Header magic --- */
#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x010B
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x020B

/* --- Data Directory --- */
typedef struct _IMAGE_DATA_DIRECTORY {
    uint32_t VirtualAddress;
    uint32_t Size;
} __attribute__((packed)) IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

/* --- Optional Header (32-bit) --- */
typedef struct _IMAGE_OPTIONAL_HEADER32 {
    uint16_t Magic;
    uint8_t  MajorLinkerVersion;
    uint8_t  MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;    /* RVA of entry point */
    uint32_t BaseOfCode;
    uint32_t BaseOfData;             /* Windows XP-era; still present in PE32 */

    /* NT additional fields */
    uint32_t ImageBase;              /* Preferred load address */
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint32_t SizeOfStackReserve;
    uint32_t SizeOfStackCommit;
    uint32_t SizeOfHeapReserve;
    uint32_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;

    /* Data directories */
    IMAGE_DATA_DIRECTORY DataDirectory[16];
} __attribute__((packed)) IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

/* --- NT Headers (32-bit) --- */
typedef struct _IMAGE_NT_HEADERS {
    uint32_t Signature;              /* "PE\0\0" = 0x00004550 */
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER32 OptionalHeader;
} __attribute__((packed)) IMAGE_NT_HEADERS32, *PIMAGE_NT_HEADERS32;

#define IMAGE_NT_SIGNATURE  0x00004550  /* "PE\0\0" */

/* --- Section Header --- */
typedef struct _IMAGE_SECTION_HEADER {
    uint8_t  Name[8];
    union {
        uint32_t PhysicalAddress;
        uint32_t VirtualSize;
    } Misc;
    uint32_t VirtualAddress;         /* RVA of section */
    uint32_t SizeOfRawData;          /* Size in file */
    uint32_t PointerToRawData;       /* File offset */
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} __attribute__((packed)) IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

/* Section characteristics */
#define IMAGE_SCN_CNT_CODE           0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA 0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080
#define IMAGE_SCN_MEM_EXECUTE        0x20000000
#define IMAGE_SCN_MEM_READ           0x40000000
#define IMAGE_SCN_MEM_WRITE          0x80000000
#define IMAGE_SCN_MEM_DISCARDABLE    0x02000000
#define IMAGE_SCN_ALIGN_4BYTES       0x00300000

/* --- Data Directory Indices --- */
#define IMAGE_DIRECTORY_ENTRY_EXPORT      0
#define IMAGE_DIRECTORY_ENTRY_IMPORT      1
#define IMAGE_DIRECTORY_ENTRY_RESOURCE    2
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION   3
#define IMAGE_DIRECTORY_ENTRY_SECURITY    4
#define IMAGE_DIRECTORY_ENTRY_BASERELOC   5
#define IMAGE_DIRECTORY_ENTRY_DEBUG       6
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE 7
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR   8
#define IMAGE_DIRECTORY_ENTRY_TLS         9
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG 10
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT 11
#define IMAGE_DIRECTORY_ENTRY_IAT         12
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT 13
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14

/* --- Import Descriptor --- */
typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    union {
        uint32_t Characteristics;    /* 0 for end-of-imports */
        uint32_t OriginalFirstThunk; /* RVA to INT (import name table) */
    } DUMMYUNIONNAME;
    uint32_t TimeDateStamp;
    uint32_t ForwarderChain;         /* -1 if no forwarders */
    uint32_t Name;                   /* RVA to DLL name string */
    uint32_t FirstThunk;             /* RVA to IAT (import address table) */
} __attribute__((packed)) IMAGE_IMPORT_DESCRIPTOR, *PIMAGE_IMPORT_DESCRIPTOR;

/* --- Import/Export Name entry --- */
typedef struct _IMAGE_IMPORT_BY_NAME {
    uint16_t Hint;                   /* Hint into export name-ptr table */
    uint8_t  Name[1];               /* Null-terminated name (variable) */
} __attribute__((packed)) IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

/* --- Thunk Data (32-bit) --- */
typedef struct _IMAGE_THUNK_DATA32 {
    union {
        uint32_t ForwarderString;    /* RVA to forwarder string */
        uint32_t Function;           /* Address of imported function */
        uint32_t Ordinal;            /* Ordinal if IMAGE_ORDINAL_FLAG set */
        uint32_t AddressOfData;      /* RVA to IMAGE_IMPORT_BY_NAME */
    } u1;
} __attribute__((packed)) IMAGE_THUNK_DATA32, *PIMAGE_THUNK_DATA32;

#define IMAGE_ORDINAL_FLAG32    0x80000000
#define IMAGE_ORDINAL32(Ordinal) ((Ordinal) & 0xFFFF)
#define IMAGE_SNAP_BY_ORDINAL32(Ordinal) \
    (((Ordinal) & IMAGE_ORDINAL_FLAG32) != 0)

/* --- Export Directory --- */
typedef struct _IMAGE_EXPORT_DIRECTORY {
    uint32_t Characteristics;
    uint32_t TimeDateStamp;
    uint16_t MajorVersion;
    uint16_t MinorVersion;
    uint32_t Name;                   /* RVA to DLL name */
    uint32_t Base;                   /* Ordinal base */
    uint32_t NumberOfFunctions;
    uint32_t NumberOfNames;
    uint32_t AddressOfFunctions;     /* RVA to array of function RVAs */
    uint32_t AddressOfNames;         /* RVA to array of name RVAs */
    uint32_t AddressOfNameOrdinals;  /* RVA to array of ordinal WORDs */
} __attribute__((packed)) IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

/* --- Base Relocation --- */
typedef struct _IMAGE_BASE_RELOCATION {
    uint32_t VirtualAddress;         /* Base RVA of the page */
    uint32_t SizeOfBlock;            /* Total size incl. header */
} __attribute__((packed)) IMAGE_BASE_RELOCATION, *PIMAGE_BASE_RELOCATION;

/* Relocation types */
#define IMAGE_REL_BASED_ABSOLUTE    0
#define IMAGE_REL_BASED_HIGH        1
#define IMAGE_REL_BASED_LOW         2
#define IMAGE_REL_BASED_HIGHLOW     3
#define IMAGE_REL_BASED_HIGHADJ     4
#define IMAGE_REL_BASED_DIR64       10

/* --- TLS Directory --- */
typedef struct _IMAGE_TLS_DIRECTORY32 {
    uint32_t StartAddressOfRawData;
    uint32_t EndAddressOfRawData;
    uint32_t AddressOfIndex;         /* PImage to TLS index */
    uint32_t AddressOfCallBacks;     /* PImage to callback array */
    uint32_t SizeOfZeroFill;
    uint32_t Characteristics;
} __attribute__((packed)) IMAGE_TLS_DIRECTORY32, *PIMAGE_TLS_DIRECTORY32;

/* --- Subsystem values --- */
#define IMAGE_SUBSYSTEM_NATIVE           1
#define IMAGE_SUBSYSTEM_WINDOWS_GUI      2
#define IMAGE_SUBSYSTEM_WINDOWS_CUI      3

/* --- DLL characteristics --- */
#define IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE  0x0040
#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT     0x0100

/* --- Helper macros --- */
#define PE_RVA_TO_OFFSET(Rva, Section, SectionCount, FileBuffer) \
    PeRvaToOffset((Rva), (Section), (SectionCount), (FileBuffer))

/* ============================================================
 *  PE Loader API - kernel-mode PE parser
 * ============================================================ */

/* Loaded module descriptor */
typedef struct _LDR_MODULE {
    struct _LDR_MODULE *Next;
    struct _LDR_MODULE *Prev;
    void               *DllBase;       /* Base address in memory */
    void               *EntryPoint;    /* DllMain address */
    uint32_t            SizeOfImage;
    uint32_t            ImageBase;
    char                BaseDllName[64];
    char                FullDllName[128];
    uint32_t            Flags;
    uint16_t            LoadCount;
} LDR_MODULE, *PLDR_MODULE;

/* PE Loader functions */
void *PeRvaToOffset(uint32_t rva, const IMAGE_SECTION_HEADER *sections,
                    uint16_t num_sections, const void *file_buffer);
int   PeValidateImage(const void *file_buffer, uint32_t file_size);
int   PeLoadImage(const void *file_buffer, uint32_t file_size,
                  void *load_base, uint32_t *size_of_image_out,
                  IMAGE_SECTION_HEADER *sections_out,
                  uint16_t *num_sections_out,
                  IMAGE_NT_HEADERS32 *nt_headers_out);
int   PeProcessRelocations(void *image_base,
                           const IMAGE_NT_HEADERS32 *nt_headers,
                           const IMAGE_SECTION_HEADER *sections);
int   PeBuildImportTable(void *image_base, PLDR_MODULE module_list,
                         const IMAGE_NT_HEADERS32 *nt_headers,
                         const IMAGE_SECTION_HEADER *sections);
void *PeFindExport(const void *image_base,
                   const IMAGE_NT_HEADERS32 *nt_headers,
                   const IMAGE_SECTION_HEADER *sections,
                   const char *name, uint16_t ordinal);

#endif /* COREOS_PE_H */
