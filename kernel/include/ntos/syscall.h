#ifndef COREOS_NTOS_SYSCALL_H
#define COREOS_NTOS_SYSCALL_H

#include "nt/ntdef.h"
#include "nt/ntstatus.h"
#include "ntos/io.h"
#include "ntos/ps.h"

/* Syscall numbers (NT-style) */
#define SYSCALL_NtCreateProcess         0x26
#define SYSCALL_NtCreateThread          0x27
#define SYSCALL_NtTerminateProcess      0x28
#define SYSCALL_NtTerminateThread       0x29
#define SYSCALL_NtCreateFile            0x32
#define SYSCALL_NtReadFile              0x3F
#define SYSCALL_NtWriteFile             0x57
#define SYSCALL_NtCloseFile             0x20
#define SYSCALL_NtOpenFile              0x30
#define SYSCALL_NtQueryInformationFile  0x36
#define SYSCALL_NtSetInformationFile    0x49
#define SYSCALL_NtExitProcess           0x2C
#define SYSCALL_NtExitThread            0x2D
#define SYSCALL_NtGetCurrentProcessId   0x40
#define SYSCALL_NtGetCurrentThreadId    0x41
#define SYSCALL_NtAllocateVirtualMemory 0x18
#define SYSCALL_NtFreeVirtualMemory     0x1E

/* File handle type */
typedef uint32_t HANDLE;

/* Syscall frame (for exception handlers) */
typedef struct _SYSCALL_FRAME {
    uint32_t eax;  /* Syscall number + return */
    uint32_t ebx, ecx, edx, esi, edi, ebp, esp;
    uint32_t eip;
    uint32_t eflags;
    uint32_t arg1, arg2, arg3, arg4;
} SYSCALL_FRAME;

/* Syscall prototypes */
void SyscallInit(void);
NTSTATUS NTAPI NtCreateProcess(PCSTR ImagePath, HANDLE *ProcessHandle);
NTSTATUS NTAPI NtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, PCSTR FileName);
NTSTATUS NTAPI NtReadFile(HANDLE FileHandle, void *Buffer, uint32_t Length, uint32_t *BytesRead);
NTSTATUS NTAPI NtWriteFile(HANDLE FileHandle, const void *Buffer, uint32_t Length, uint32_t *BytesWritten);
NTSTATUS NTAPI NtCloseFile(HANDLE FileHandle);
NTSTATUS NTAPI NtGetCurrentProcessId(uint32_t *ProcessId);
NTSTATUS NTAPI NtGetCurrentThreadId(uint32_t *ThreadId);
NTSTATUS NTAPI NtExitProcess(uint32_t ExitCode);
NTSTATUS NTAPI NtExitThread(uint32_t ExitCode);

#endif
