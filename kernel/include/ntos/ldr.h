#ifndef COREOS_NTOS_LDR_H
#define COREOS_NTOS_LDR_H

#include "nt/ntdef.h"
#include "nt/ntstatus.h"
#include "pe.h"

/* ============================================================
 *  Ldr — Windows NT-style Module Loader
 *  Manages loading, lookup, and unloading of PE images (DLLs/EXEs)
 * ============================================================ */

/* Ldr flags */
#define LDR_FLAGS_LOADED            0x00000001
#define LDR_FLAGS_IMAGE_DLL         0x00000002
#define LDR_FLAGS_ENTRY_POINT_CALLED 0x00000004
#define LDR_FLAGS_RESOURCE_ONLY     0x00000008
#define LDR_FLAGS_SYSTEM_MAPPED     0x00000010

/* Maximum number of loaded modules */
#define LDR_MAX_MODULES 64

/* --- Initialization --- */
void LdrInitSystem(void);

/* --- Core API --- */
/* Load a DLL into memory. Returns handle or NULL on failure. */
void *LdrLoadDll(const char *dll_path, const char *dll_name);

/* Get the address of an exported function from a loaded module. */
void *LdrGetProcedureAddress(void *module_handle, const char *function_name);

/* Unload a previously loaded DLL. Returns NTSTATUS. */
NTSTATUS LdrUnloadDll(void *module_handle);

/* Find a loaded module by address or name. */
PLDR_MODULE LdrFindEntryForAddress(const void *address);
PLDR_MODULE LdrFindEntryForName(const char *dll_name);

/* Get module handle by name (returns handle for GetModuleHandle). */
void *LdrGetModuleHandle(const char *module_name);

/* Enumerate loaded modules (for debugging). */
uint32_t LdrGetModuleCount(void);
PLDR_MODULE LdrGetModuleList(void);

/* --- Internal helpers --- */
/* Allocate memory for loading a PE image. */
void *LdrAllocateImageMemory(uint32_t size);

/* Free memory used by a PE image. */
void LdrFreeImageMemory(void *base, uint32_t size);

/* Call DllMain entry point with given reason. */
NTSTATUS LdrCallDllEntry(void *entry, void *dll_base, uint32_t reason, void *reserved);

/* Resolve a single import (lookup function name in loaded modules). */
void *LdrResolveImport(const char *dll_name, const char *func_name, uint32_t ordinal);

#endif /* COREOS_NTOS_LDR_H */
