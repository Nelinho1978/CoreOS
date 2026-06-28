#include "ntos/syscall.h"
#include "ntos/io.h"
#include "ntos/ps.h"
#include "ntos/fat32.h"
#include "coreos/printk.h"
#include "arch/irq.h"

/* File handle table */
#define MAX_HANDLES 256

typedef struct {
    uint32_t valid;
    FAT32_FILE file;
    uint32_t access_mode;
} HANDLE_ENTRY;

static HANDLE_ENTRY g_HandleTable[MAX_HANDLES];
static uint32_t g_NextHandle = 1;

/* Syscall dispatcher */
extern void x86_syscall_handler(SYSCALL_FRAME *frame);

void SyscallDispatcher(SYSCALL_FRAME *frame) {
    NTSTATUS status = STATUS_SUCCESS;
    uint32_t syscall_num = frame->eax & 0xFFFF;
    
    kprintf("[Syscall] Number: 0x%X\n", syscall_num);
    
    switch (syscall_num) {
        case SYSCALL_NtCreateProcess:
            status = NtCreateProcess((PCSTR)frame->arg1, (HANDLE *)frame->arg2);
            break;
        case SYSCALL_NtCreateFile:
            status = NtCreateFile((PHANDLE)frame->arg1, frame->arg2, (PCSTR)frame->arg3);
            break;
        case SYSCALL_NtReadFile:
            status = NtReadFile((HANDLE)frame->arg1, (void *)frame->arg2, frame->arg3, (uint32_t *)frame->arg4);
            break;
        case SYSCALL_NtWriteFile:
            status = NtWriteFile((HANDLE)frame->arg1, (void *)frame->arg2, frame->arg3, (uint32_t *)frame->arg4);
            break;
        case SYSCALL_NtCloseFile:
            status = NtCloseFile((HANDLE)frame->arg1);
            break;
        case SYSCALL_NtGetCurrentProcessId:
            status = NtGetCurrentProcessId((uint32_t *)frame->arg1);
            break;
        case SYSCALL_NtGetCurrentThreadId:
            status = NtGetCurrentThreadId((uint32_t *)frame->arg1);
            break;
        case SYSCALL_NtExitProcess:
            status = NtExitProcess(frame->arg1);
            break;
        case SYSCALL_NtExitThread:
            status = NtExitThread(frame->arg1);
            break;
        default:
            kprintf("[Syscall] Unknown syscall: 0x%X\n", syscall_num);
            status = STATUS_NOT_IMPLEMENTED;
    }
    
    frame->eax = status;
}

NTSTATUS NTAPI NtCreateProcess(PCSTR ImagePath, HANDLE *ProcessHandle) {
    if (!ImagePath || !ProcessHandle)
        return STATUS_INVALID_PARAMETER;
    
    EPROCESS *proc = NULL;
    NTSTATUS status = PsCreateProcess(ImagePath, &proc);
    if (!NT_SUCCESS(status))
        return status;
    
    if (!proc)
        return STATUS_NO_MEMORY;
    
    /* Allocate handle */
    uint32_t handle_idx = g_NextHandle++;
    if (handle_idx >= MAX_HANDLES)
        return STATUS_TOO_MANY_OPENED_FILES;
    
    *ProcessHandle = (HANDLE)handle_idx;
    
    kprintf("[NtCreateProcess] Created process: %s, Handle=%u, PID=%u\n", ImagePath, handle_idx, proc->ProcessId);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtCreateFile(PHANDLE FileHandle, ACCESS_MASK DesiredAccess, PCSTR FileName) {
    if (!FileHandle || !FileName)
        return STATUS_INVALID_PARAMETER;
    
    if (g_NextHandle >= MAX_HANDLES)
        return STATUS_TOO_MANY_OPENED_FILES;
    
    FAT32_FILE *file = &g_HandleTable[g_NextHandle].file;
    
    if (!fat32_open(FileName, file)) {
        kprintf("[NtCreateFile] File not found: %s\n", FileName);
        return STATUS_FILE_NOT_FOUND;
    }
    
    HANDLE handle = g_NextHandle;
    g_HandleTable[g_NextHandle].valid = 1;
    g_HandleTable[g_NextHandle].access_mode = DesiredAccess;
    g_NextHandle++;
    
    *FileHandle = handle;
    
    kprintf("[NtCreateFile] Opened file: %s, Handle=%u\n", FileName, handle);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtReadFile(HANDLE FileHandle, void *Buffer, uint32_t Length, uint32_t *BytesRead) {
    if (!Buffer || !BytesRead || FileHandle >= MAX_HANDLES)
        return STATUS_INVALID_PARAMETER;
    
    HANDLE_ENTRY *entry = &g_HandleTable[FileHandle];
    if (!entry->valid)
        return STATUS_INVALID_HANDLE;
    
    uint32_t bytes = 0;
    if (!fat32_read(&entry->file, Buffer, Length, &bytes)) {
        kprintf("[NtReadFile] Read failed on handle %u\n", FileHandle);
        return STATUS_IO_DEVICE_ERROR;
    }
    
    *BytesRead = bytes;
    kprintf("[NtReadFile] Read %u bytes from handle %u\n", bytes, FileHandle);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtWriteFile(HANDLE FileHandle, const void *Buffer, uint32_t Length, uint32_t *BytesWritten) {
    if (!Buffer || !BytesWritten || FileHandle >= MAX_HANDLES)
        return STATUS_INVALID_PARAMETER;
    
    HANDLE_ENTRY *entry = &g_HandleTable[FileHandle];
    if (!entry->valid)
        return STATUS_INVALID_HANDLE;
    
    /* TODO: Implement FAT32 write */
    kprintf("[NtWriteFile] Write requested on handle %u (length=%u)\n", FileHandle, Length);
    
    *BytesWritten = Length;  /* Fake write for now */
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtCloseFile(HANDLE FileHandle) {
    if (FileHandle >= MAX_HANDLES)
        return STATUS_INVALID_PARAMETER;
    
    HANDLE_ENTRY *entry = &g_HandleTable[FileHandle];
    if (!entry->valid)
        return STATUS_INVALID_HANDLE;
    
    fat32_close(&entry->file);
    entry->valid = 0;
    
    kprintf("[NtCloseFile] Closed handle %u\n", FileHandle);
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtGetCurrentProcessId(uint32_t *ProcessId) {
    if (!ProcessId)
        return STATUS_INVALID_PARAMETER;
    
    EPROCESS *proc = PsGetCurrentProcess();
    if (!proc)
        return STATUS_INTERNAL_ERROR;
    
    *ProcessId = proc->ProcessId;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtGetCurrentThreadId(uint32_t *ThreadId) {
    if (!ThreadId)
        return STATUS_INVALID_PARAMETER;
    
    KTHREAD *thread = PsGetCurrentThread();
    if (!thread)
        return STATUS_INTERNAL_ERROR;
    
    *ThreadId = thread->ThreadId;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI NtExitProcess(uint32_t ExitCode) {
    EPROCESS *proc = PsGetCurrentProcess();
    if (!proc)
        return STATUS_INTERNAL_ERROR;
    
    return PsTerminateProcess(proc, ExitCode);
}

NTSTATUS NTAPI NtExitThread(uint32_t ExitCode) {
    KTHREAD *thread = PsGetCurrentThread();
    if (!thread)
        return STATUS_INTERNAL_ERROR;
    
    return PsTerminateThread(thread, ExitCode);
}

void SyscallInit(void) {
    int i;
    for (i = 0; i < MAX_HANDLES; ++i) {
        g_HandleTable[i].valid = 0;
    }
    kputs("[Syscall] System call interface initialized\n");
}
