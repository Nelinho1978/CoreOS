#include "subsys/win32_stubs.h"
#include "coreos/printk.h"
#include "string.h"

/* ============================================================
 *  Win32 Stub DLLs — Built-in implementations of core Win32 API
 *
 *  These stubs provide enough functionality for PE images
 *  to load and run basic initialization code. Each function
 *  logs its call via serial and returns a reasonable default.
 *
 *  DLLs covered:
 *    - ntdll.dll   (Rtl* heap/string/critical section functions)
 *    - kernel32.dll (Heap, Virtual*, Process/Thread, Library loading)
 *    - user32.dll   (Window/Message/Input functions)
 *    - gdi32.dll    (Graphics/Display functions)
 * ============================================================ */

/* ---- Stub dispatch table ---- */
typedef struct {
    const char *dll_name;
    const char *func_name;
    void *func_ptr;
} STUB_ENTRY;

/* ---- Forward declarations to avoid extern ---- */
/* (all functions are prototyped in win32_stubs.h) */

/* ============================================================
 *  NTDLL Implementation
 * ============================================================ */
static uint32_t g_last_error;
static uint32_t g_tls_slots[64];

void *NTAPI RtlAllocateHeap(void *heap, uint32_t flags, uint32_t size) {
    (void)heap; (void)flags;
    /* Fallback: use our kernel allocator for small blocks */
    extern void *MmAllocatePool(uint32_t bytes);
    void *ptr = NULL;
    if (size < 4096) {
        ptr = MmAllocatePool(size + sizeof(uint32_t));
        if (ptr) {
            *(uint32_t *)ptr = size;
            ptr = (uint8_t *)ptr + sizeof(uint32_t);
        }
    }
    return ptr;
}

int NTAPI RtlFreeHeap(void *heap, uint32_t flags, void *ptr) {
    (void)heap; (void)flags;
    if (ptr) {
        extern void MmFreePool(void *p);
        MmFreePool((uint8_t *)ptr - sizeof(uint32_t));
    }
    return 1;
}

void NTAPI RtlZeroMemory(void *ptr, uint32_t size) {
    uint8_t *p = (uint8_t *)ptr;
    uint32_t i;
    for (i = 0; i < size; ++i) p[i] = 0;
}

void NTAPI RtlCopyMemory(void *dst, const void *src, uint32_t size) {
    uint32_t i;
    for (i = 0; i < size; ++i)
        ((uint8_t *)dst)[i] = ((const uint8_t *)src)[i];
}

void NTAPI RtlMoveMemory(void *dst, const void *src, uint32_t size) {
    /* Simple byte copy — handles non-overlapping correctly */
    uint32_t i;
    if (dst < src) {
        for (i = 0; i < size; ++i)
            ((uint8_t *)dst)[i] = ((const uint8_t *)src)[i];
    } else {
        for (i = size; i > 0; --i)
            ((uint8_t *)dst)[i-1] = ((const uint8_t *)src)[i-1];
    }
}

int NTAPI RtlCompareMemory(const void *a, const void *b, uint32_t size) {
    uint32_t i;
    for (i = 0; i < size; ++i) {
        if (((const uint8_t *)a)[i] != ((const uint8_t *)b)[i])
            return (int)i;
    }
    return (int)size;
}

void NTAPI RtlFillMemory(void *ptr, uint32_t size, uint8_t fill) {
    uint32_t i;
    for (i = 0; i < size; ++i)
        ((uint8_t *)ptr)[i] = fill;
}

uint32_t NTAPI RtlGetLastError(void) {
    return g_last_error;
}

void NTAPI RtlSetLastError(uint32_t error) {
    g_last_error = error;
}

int NTAPI RtlInitializeCriticalSection(void *cs) {
    if (cs) {
        uint32_t *lock = (uint32_t *)cs;
        lock[0] = 0; /* lock count */
        lock[1] = 0; /* recursion count */
        lock[2] = 0; /* owning thread */
    }
    return 1;
}

int NTAPI RtlEnterCriticalSection(void *cs) {
    (void)cs;
    return 1;
}

int NTAPI RtlLeaveCriticalSection(void *cs) {
    (void)cs;
    return 1;
}

void NTAPI RtlAcquirePebLock(void) {}
void NTAPI RtlReleasePebLock(void) {}

/* ============================================================
 *  KERNEL32 Implementation
 * ============================================================ */
void *NTAPI GetModuleHandleA(const char *module_name) {
    extern void *LdrGetModuleHandle(const char *name);
    return LdrGetModuleHandle(module_name);
}

void *NTAPI GetProcAddress(void *module, const char *name) {
    extern void *LdrGetProcedureAddress(void *handle, const char *func);
    return LdrGetProcedureAddress(module, name);
}

void *NTAPI LoadLibraryA(const char *lib_name) {
    extern void *LdrLoadDll(const char *path, const char *name);
    kputs("[KERNEL32] LoadLibraryA: ");
    kputs(lib_name ? lib_name : "(null)");
    kputs("\n");
    return LdrLoadDll(lib_name, NULL);
}

int NTAPI FreeLibrary(void *module) {
    extern NTSTATUS LdrUnloadDll(void *handle);
    return NT_SUCCESS(LdrUnloadDll(module)) ? 1 : 0;
}

void *NTAPI VirtualAlloc(void *addr, uint32_t size, uint32_t type, uint32_t prot) {
    (void)type; (void)prot;
    extern void *MmAllocatePool(uint32_t bytes);
    kputs("[KERNEL32] VirtualAlloc: ");
    {
        char buf[16];
        int j;
        for (j = 0; j < 8; ++j) {
            uint32_t nibble = (size >> (28 - j*4)) & 0xF;
            buf[j] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
        }
        buf[8] = '\0';
        kputs(buf);
    }
    kputs(" bytes\n");

    if (addr) return addr; /* pretend we used the requested address */
    return MmAllocatePool(size);
}

int NTAPI VirtualFree(void *addr, uint32_t size, uint32_t type) {
    (void)size; (void)type;
    extern void MmFreePool(void *ptr);
    if (addr) MmFreePool(addr);
    return 1;
}

void *NTAPI HeapAlloc(void *heap, uint32_t flags, uint32_t size) {
    return RtlAllocateHeap(heap, flags, size);
}

int NTAPI HeapFree(void *heap, uint32_t flags, void *ptr) {
    return RtlFreeHeap(heap, flags, ptr);
}

static int g_heap_mem[256]; /* dummy heap */
void *NTAPI GetProcessHeap(void) {
    return (void *)g_heap_mem;
}

void NTAPI Sleep(uint32_t ms) {
    /* Busy-wait approximation */
    volatile uint32_t i, j;
    kputs("[KERNEL32] Sleep: ");
    {
        char buf[12];
        uint32_t n = ms;
        int j2;
        for (j2 = 9; j2 >= 0; --j2) {
            buf[j2] = '0' + (n % 10);
            n /= 10;
            if (n == 0) { buf[10] = '\0'; break; }
        }
    }
    kputs("ms\n");
    for (i = 0; i < ms * 1000; ++i) { for (j = 0; j < 10; ++j) { __asm__ volatile("pause"); } }
}

uint32_t NTAPI GetTickCount(void) {
    /* Return a fake tick count that increments */
    static uint32_t ticks = 0;
    ticks += 16;
    return ticks;
}

int NTAPI GetSystemInfo(void *info) {
    /* Fill in SYSTEM_INFO structure */
    if (info) {
        uint32_t *p = (uint32_t *)info;
        p[0] = 0x0804;  /* wProcessorArchitecture = x86 */
        p[1] = 0;        /* dwPageSize = 4096 */
        p[2] = NULL;      /* lpMinimumApplicationAddress */
        p[3] = (uint32_t)0x7FFFFFFF; /* lpMaximumApplicationAddress */
        p[4] = 0x100000000ULL; /* dwActiveProcessorMask */
        p[5] = 1;        /* dwNumberOfProcessors */
        p[6] = 0;        /* dwProcessorType */
        p[7] = 0;        /* dwAllocationGranularity = 64KB */
        p[8] = 0;        /* wProcessorLevel */
        p[9] = 0;        /* wProcessorRevision */
    }
    return 0;
}

void *NTAPI GetCommandLineA(void) {
    static char cmd[] = "CoreOS";
    kputs("[KERNEL32] GetCommandLineA\n");
    return cmd;
}

void *NTAPI GetModuleHandleW(const void *name) {
    (void)name;
    return GetModuleHandleA(NULL);
}

void *NTAPI LoadLibraryW(const void *name) {
    if (name) {
        /* Convert WCHAR* to char* — simple ASCII approximation */
        static char ansi[260];
        const uint16_t *w = (const uint16_t *)name;
        int i;
        for (i = 0; i < 259 && w[i] != 0; ++i)
            ansi[i] = (char)(w[i] & 0xFF);
        ansi[i] = '\0';
        return LoadLibraryA(ansi);
    }
    return NULL;
}

int NTAPI GetExitCodeProcess(void *process, uint32_t *code) {
    (void)process;
    if (code) *code = 0;
    return 1;
}

void NTAPI ExitProcess(uint32_t code) {
    kputs("[KERNEL32] ExitProcess: ");
    {
        char buf[12];
        uint32_t n = code;
        int i;
        buf[11] = '\0';
        for (i = 10; i >= 0; --i) { buf[i] = '0' + (n % 10); n /= 10; if (n == 0) break; }
    }
    kputs("\n");
    for (;;) { __asm__ volatile("hlt"); }
}

void *NTAPI CreateThread(void *attr, uint32_t stack, void *start, void *param,
                          uint32_t flags, uint32_t *id) {
    kputs("[KERNEL32] CreateThread (stub)\n");
    (void)attr; (void)stack; (void)start; (void)param; (void)flags;
    if (id) *id = 2;
    /* Return a dummy handle */
    static uint32_t dummy;
    return &dummy;
}

uint32_t NTAPI WaitForSingleObject(void *handle, uint32_t ms) {
    (void)handle; (void)ms;
    kputs("[KERNEL32] WaitForSingleObject (stub)\n");
    return 0; /* WAIT_OBJECT_0 */
}

int NTAPI CloseHandle(void *handle) {
    (void)handle;
    return 1;
}

uint32_t NTAPI GetLastError(void) {
    return RtlGetLastError();
}

void NTAPI SetLastError(uint32_t error) {
    RtlSetLastError(error);
}

void NTAPI OutputDebugStringA(const char *str) {
    kputs("[DBG] ");
    kputs(str ? str : "(null)");
    kputs("\n");
}

void NTAPI OutputDebugStringW(const void *str) {
    OutputDebugStringA("(wide string)");
}

uint32_t NTAPI GetModuleFileNameA(void *module, char *buf, uint32_t size) {
    extern PLDR_MODULE LdrFindEntryForAddress(const void *addr);
    PLDR_MODULE mod = LdrFindEntryForAddress(module);
    if (mod && buf && size > 0) {
        uint32_t i;
        const char *src = mod->FullDllName;
        for (i = 0; i < size - 1 && src[i]; ++i) buf[i] = src[i];
        buf[i] = '\0';
        return i;
    }
    if (buf && size > 0) buf[0] = '\0';
    return 0;
}

void *NTAPI LocalAlloc(uint32_t flags, uint32_t size) {
    return RtlAllocateHeap(NULL, flags, size);
}

void *NTAPI LocalFree(void *ptr) {
    if (ptr) RtlFreeHeap(NULL, 0, ptr);
    return NULL;
}

int NTAPI QueryPerformanceCounter(void *counter) {
    if (counter) {
        static uint32_t counter_val = 0;
        counter_val += 1000;
        *(uint64_t *)counter = counter_val;
    }
    return 1;
}

int NTAPI QueryPerformanceFrequency(void *freq) {
    if (freq) *(uint64_t *)freq = 1000000;
    return 1;
}

void *NTAPI GetCurrentProcess(void) {
    static uint32_t proc = 0xDEAD0001;
    return &proc;
}

void *NTAPI GetCurrentThread(void) {
    static uint32_t thread = 0xDEAD0002;
    return &thread;
}

uint32_t NTAPI GetCurrentProcessId(void) {
    return 4; /* PID 4 = System */
}

uint32_t NTAPI GetCurrentThreadId(void) {
    return 1; /* TID 1 = System thread */
}

int NTAPI IsProcessorFeaturePresent(uint32_t feature) {
    (void)feature;
    return 1; /* Pretend all features are present */
}

void NTAPI InitializeCriticalSection(void *cs) {
    RtlInitializeCriticalSection(cs);
}

void NTAPI EnterCriticalSection(void *cs) {
    RtlEnterCriticalSection(cs);
}

void NTAPI LeaveCriticalSection(void *cs) {
    RtlLeaveCriticalSection(cs);
}

int NTAPI InitializeCriticalSectionAndSpinCount(void *cs, uint32_t count) {
    (void)count;
    return RtlInitializeCriticalSection(cs);
}

int NTAPI TryEnterCriticalSection(void *cs) {
    (void)cs;
    return 1;
}

void NTAPI DeleteCriticalSection(void *cs) {
    (void)cs;
}

int NTAPI DisableThreadLibraryCalls(void *module) {
    (void)module;
    return 1;
}

void *NTAPI GetModuleHandleExA(uint32_t flags, const char *name, void **module) {
    void *h = GetModuleHandleA(name);
    if (module) *module = h;
    return h ? 1 : 0;
}

void *NTAPI TlsGetValue(uint32_t index) {
    if (index < 64) return (void *)g_tls_slots[index];
    return NULL;
}

int NTAPI TlsSetValue(uint32_t index, void *value) {
    if (index < 64) { g_tls_slots[index] = (uint32_t)value; return 1; }
    return 0;
}

uint32_t NTAPI TlsAlloc(void) {
    uint32_t i;
    for (i = 0; i < 64; ++i) {
        if (g_tls_slots[i] == 0xFFFFFFFF) {
            g_tls_slots[i] = 0;
            return i;
        }
    }
    return 0xFFFFFFFF;
}

int NTAPI TlsFree(uint32_t index) {
    if (index < 64) { g_tls_slots[index] = 0xFFFFFFFF; return 1; }
    return 0;
}

void *NTAPI GetStartupInfoA(void *info) {
    kputs("[KERNEL32] GetStartupInfoA (stub)\n");
    if (info) RtlZeroMemory(info, 68);
    return info;
}

/* ============================================================
 *  USER32 Implementation
 * ============================================================ */
int NTAPI MessageBoxA(void *hwnd, const char *text, const char *caption, uint32_t type) {
    (void)hwnd; (void)type;
    kputs("[USER32] MessageBoxA: \"");
    kputs(caption ? caption : "");
    kputs("\" - \"");
    kputs(text ? text : "");
    kputs("\"\n");
    return 1; /* IDOK */
}

int NTAPI MessageBoxW(void *hwnd, const void *text, const void *caption, uint32_t type) {
    (void)hwnd; (void)text; (void)caption; (void)type;
    kputs("[USER32] MessageBoxW (stub)\n");
    return 1;
}

void *NTAPI CreateWindowExA(uint32_t ex, const char *cls, const char *title,
                            uint32_t style, int x, int y, int w, int h,
                            void *parent, void *menu, void *inst, void *param) {
    (void)ex; (void)cls; (void)title; (void)style; (void)x; (void)y; (void)w; (void)h;
    (void)parent; (void)menu; (void)inst; (void)param;
    kputs("[USER32] CreateWindowExA: ");
    kputs(cls ? cls : "(null)");
    kputs(" - ");
    kputs(title ? title : "(null)");
    kputs("\n");
    /* Return a dummy window handle */
    static uint32_t hwnd_val = 0xCAFE0001;
    return (void *)(hwnd_val++);
}

void *NTAPI CreateWindowExW(uint32_t ex, const void *cls, const void *title,
                            uint32_t style, int x, int y, int w, int h,
                            void *parent, void *menu, void *inst, void *param) {
    (void)ex; (void)cls; (void)title; (void)style; (void)x; (void)y; (void)w; (void)h;
    (void)parent; (void)menu; (void)inst; (void)param;
    kputs("[USER32] CreateWindowExW (stub)\n");
    static uint32_t hwnd_val = 0xCAFE1001;
    return (void *)(hwnd_val++);
}

int NTAPI DestroyWindow(void *hwnd) {
    (void)hwnd;
    return 1;
}

int NTAPI ShowWindow(void *hwnd, int cmd) {
    (void)hwnd; (void)cmd;
    return 1;
}

int NTAPI UpdateWindow(void *hwnd) {
    (void)hwnd;
    return 1;
}

int NTAPI GetMessageA(void *msg, void *hwnd, uint32_t min, uint32_t max) {
    (void)msg; (void)hwnd; (void)min; (void)max;
    /* Block forever — no real message queue */
    kputs("[USER32] GetMessageA (stub — bloqueando)\n");
    for (;;) { __asm__ volatile("pause"); }
    return 0;
}

int NTAPI PeekMessageA(void *msg, void *hwnd, uint32_t min, uint32_t max, uint32_t remove) {
    (void)msg; (void)hwnd; (void)min; (void)max; (void)remove;
    return 0; /* No messages */
}

int NTAPI TranslateMessage(const void *msg) {
    (void)msg;
    return 0;
}

int NTAPI DispatchMessageA(const void *msg) {
    (void)msg;
    return 0;
}

void *NTAPI DefWindowProcA(void *hwnd, uint32_t msg, uint32_t wp, uint32_t lp) {
    (void)hwnd; (void)msg; (void)wp; (void)lp;
    return 0;
}

void *NTAPI DefWindowProcW(void *hwnd, uint32_t msg, uint32_t wp, uint32_t lp) {
    (void)hwnd; (void)msg; (void)wp; (void)lp;
    return 0;
}

void *NTAPI RegisterClassA(const void *cls) {
    (void)cls;
    kputs("[USER32] RegisterClassA\n");
    static uint16_t atom = 0xC000;
    return (void *)(uint32_t)(++atom);
}

void *NTAPI RegisterClassW(const void *cls) {
    (void)cls;
    return RegisterClassA(NULL);
}

void *NTAPI LoadCursorA(void *inst, const char *name) {
    (void)inst; (void)name;
    static uint32_t cursor = 0xF00F0001;
    return (void *)cursor;
}

void *NTAPI LoadIconA(void *inst, const char *name) {
    (void)inst; (void)name;
    static uint32_t icon = 0xF00F0002;
    return (void *)icon;
}

int NTAPI GetClientRect(void *hwnd, void *rect) {
    (void)hwnd;
    if (rect) {
        uint32_t *r = (uint32_t *)rect;
        r[0] = 0;      /* left */
        r[1] = 0;      /* top */
        r[2] = 800;    /* right */
        r[3] = 600;    /* bottom */
    }
    return 1;
}

int NTAPI GetWindowRect(void *hwnd, void *rect) {
    (void)hwnd;
    if (rect) return GetClientRect(NULL, rect);
    return 0;
}

int NTAPI AdjustWindowRect(void *rect, uint32_t style, int menu) {
    (void)style; (void)menu;
    (void)rect;
    return 1;
}

int NTAPI MoveWindow(void *hwnd, int x, int y, int w, int h, int repaint) {
    (void)hwnd; (void)x; (void)y; (void)w; (void)h; (void)repaint;
    return 1;
}

int NTAPI SetWindowTextA(void *hwnd, const char *text) {
    (void)hwnd; (void)text;
    return 1;
}

int NTAPI GetWindowTextA(void *hwnd, char *buf, int max) {
    (void)hwnd;
    if (buf && max > 0) { buf[0] = '\0'; }
    return 0;
}

uint32_t NTAPI GetWindowTextLengthA(void *hwnd) {
    (void)hwnd;
    return 0;
}

int NTAPI InvalidateRect(void *hwnd, const void *rect, int erase) {
    (void)hwnd; (void)rect; (void)erase;
    return 1;
}

void *NTAPI BeginPaint(void *hwnd, void *ps) {
    (void)hwnd;
    if (ps) RtlZeroMemory(ps, 64);
    static uint32_t dc = 0xDC000001;
    return &dc;
}

int NTAPI EndPaint(void *hwnd, const void *ps) {
    (void)hwnd; (void)ps;
    return 1;
}

void *NTAPI GetDC(void *hwnd) {
    (void)hwnd;
    static uint32_t dc = 0xDC000002;
    return &dc;
}

int NTAPI ReleaseDC(void *hwnd, void *dc) {
    (void)hwnd; (void)dc;
    return 1;
}

int NTAPI PostQuitMessage(int code) {
    (void)code;
    return 0;
}

void *NTAPI SetFocus(void *hwnd) {
    (void)hwnd;
    return NULL;
}

void *NTAPI GetFocus(void) {
    return NULL;
}

int NTAPI IsWindow(void *hwnd) {
    (void)hwnd;
    return 1;
}

int NTAPI EnableWindow(void *hwnd, int enable) {
    (void)hwnd; (void)enable;
    return 1;
}

int NTAPI SetTimer(void *hwnd, uint32_t id, uint32_t ms, void *proc) {
    (void)hwnd; (void)id; (void)ms; (void)proc;
    return 1;
}

int NTAPI KillTimer(void *hwnd, uint32_t id) {
    (void)hwnd; (void)id;
    return 1;
}

int NTAPI DrawTextA(void *dc, const char *text, int count, void *rect, uint32_t fmt) {
    (void)dc; (void)text; (void)count; (void)rect; (void)fmt;
    return 1;
}

int NTAPI FillRect(void *dc, const void *rect, void *brush) {
    (void)dc; (void)rect; (void)brush;
    return 1;
}

int NTAPI SetBkColor(void *dc, uint32_t color) {
    (void)dc; (void)color;
    return 1;
}

int NTAPI SetTextColor(void *dc, uint32_t color) {
    (void)dc; (void)color;
    return 1;
}

uint32_t NTAPI MapVirtualKeyA(uint32_t code, uint32_t type) {
    (void)code; (void)type;
    return 0;
}

int NTAPI GetKeyState(int key) {
    (void)key;
    return 0;
}

int NTAPI GetAsyncKeyState(int key) {
    (void)key;
    return 0;
}

void *NTAPI SetCapture(void *hwnd) {
    (void)hwnd;
    return NULL;
}

int NTAPI ReleaseCapture(void) {
    return 1;
}

void *NTAPI GetCapture(void) {
    return NULL;
}

void *NTAPI WindowFromPoint(int x, int y) {
    (void)x; (void)y;
    return NULL;
}

void *NTAPI ChildWindowFromPoint(void *parent, int x, int y) {
    (void)parent; (void)x; (void)y;
    return NULL;
}

int NTAPI ScreenToClient(void *hwnd, void *point) {
    (void)hwnd; (void)point;
    return 1;
}

int NTAPI ClientToScreen(void *hwnd, void *point) {
    (void)hwnd; (void)point;
    return 1;
}

void *NTAPI LoadBitmapA(void *inst, const char *name) {
    (void)inst; (void)name;
    return LoadIconA(NULL, NULL);
}

void *NTAPI LoadImageA(void *inst, const char *name, uint32_t type,
                        int cx, int cy, uint32_t flags) {
    (void)inst; (void)name; (void)type; (void)cx; (void)cy; (void)flags;
    return NULL;
}

void *NTAPI LoadCursorW(void *inst, const void *name) {
    (void)inst; (void)name;
    return LoadCursorA(NULL, NULL);
}

void *NTAPI LoadIconW(void *inst, const void *name) {
    (void)inst; (void)name;
    return LoadIconA(NULL, NULL);
}

int NTAPI OpenClipboard(void *hwnd) {
    (void)hwnd;
    return 1;
}

int NTAPI CloseClipboard(void) {
    return 1;
}

int NTAPI EmptyClipboard(void) {
    return 1;
}

void *NTAPI SetClipboardData(uint32_t fmt, void *data) {
    (void)fmt; (void)data;
    return data;
}

void *NTAPI GetClipboardData(uint32_t fmt) {
    (void)fmt;
    return NULL;
}

/* ============================================================
 *  GDI32 Implementation
 * ============================================================ */
int NTAPI BitBlt(void *dc, int x, int y, int w, int h,
                 void *src_dc, int src_x, int src_y, uint32_t op) {
    (void)dc; (void)x; (void)y; (void)w; (void)h;
    (void)src_dc; (void)src_x; (void)src_y; (void)op;
    return 1;
}

int NTAPI StretchBlt(void *dc, int x, int y, int w, int h,
                     void *src_dc, int sx, int sy, int sw, int sh, uint32_t op) {
    (void)dc; (void)x; (void)y; (void)w; (void)h;
    (void)src_dc; (void)sx; (void)sy; (void)sw; (void)sh; (void)op;
    return 1;
}

int NTAPI PatBlt(void *dc, int x, int y, int w, int h, uint32_t op) {
    (void)dc; (void)x; (void)y; (void)w; (void)h; (void)op;
    return 1;
}

void *NTAPI CreateCompatibleDC(void *dc) {
    (void)dc;
    kputs("[GDI32] CreateCompatibleDC\n");
    static uint32_t dc_val = 0xDD000001;
    return &dc_val;
}

int NTAPI DeleteDC(void *dc) {
    (void)dc;
    return 1;
}

void *NTAPI SelectObject(void *dc, void *obj) {
    (void)dc; (void)obj;
    return obj; /* return the same object */
}

int NTAPI DeleteObject(void *obj) {
    (void)obj;
    return 1;
}

void *NTAPI CreateCompatibleBitmap(void *dc, int w, int h) {
    (void)dc; (void)w; (void)h;
    kputs("[GDI32] CreateCompatibleBitmap\n");
    static uint32_t bmp = 0xBB000001;
    return &bmp;
}

void *NTAPI CreateSolidBrush(uint32_t color) {
    (void)color;
    static uint32_t brush = 0xBB000010;
    return &brush;
}

void *NTAPI CreatePen(int style, int width, uint32_t color) {
    (void)style; (void)width; (void)color;
    static uint32_t pen = 0xBB000020;
    return &pen;
}

void *NTAPI CreateFontA(int h, int w, int esc, int orient, uint32_t weight,
                        uint32_t italic, uint32_t uline, uint32_t strike,
                        uint32_t charset, uint32_t out, uint32_t clip,
                        uint32_t qual, const char *face) {
    (void)h; (void)w; (void)esc; (void)orient; (void)weight;
    (void)italic; (void)uline; (void)strike; (void)charset;
    (void)out; (void)clip; (void)qual; (void)face;
    static uint32_t font = 0xBB000030;
    return &font;
}

int NTAPI GetDeviceCaps(void *dc, int index) {
    (void)dc;
    switch (index) {
        case 0: return 800;   /* HORZRES */
        case 1: return 600;   /* VERTRES */
        case 2: return 96;    /* LOGPIXELSX */
        case 3: return 96;    /* LOGPIXELSY */
        case 4: return 32;    /* BITSPIXEL */
        case 5: return 1;     /* PLANES */
        default: return 0;
    }
}

int NTAPI Rectangle(void *dc, int left, int top, int right, int bottom) {
    (void)dc; (void)left; (void)top; (void)right; (void)bottom;
    return 1;
}

int NTAPI Ellipse(void *dc, int left, int top, int right, int bottom) {
    (void)dc; (void)left; (void)top; (void)right; (void)bottom;
    return 1;
}

int NTAPI MoveToEx(void *dc, int x, int y, void *old) {
    (void)dc; (void)x; (void)y; (void)old;
    return 1;
}

int NTAPI LineTo(void *dc, int x, int y) {
    (void)dc; (void)x; (void)y;
    return 1;
}

int NTAPI SetPixel(void *dc, int x, int y, uint32_t color) {
    (void)dc; (void)x; (void)y; (void)color;
    return 1;
}

uint32_t NTAPI GetPixel(void *dc, int x, int y) {
    (void)dc; (void)x; (void)y;
    return 0;
}

int NTAPI TextOutA(void *dc, int x, int y, const char *text, int count) {
    (void)dc; (void)x; (void)y; (void)text; (void)count;
    return 1;
}

int NTAPI TextOutW(void *dc, int x, int y, const void *text, int count) {
    (void)dc; (void)x; (void)y; (void)text; (void)count;
    return 1;
}

int NTAPI SetBkMode(void *dc, int mode) {
    (void)dc; (void)mode;
    return 1;
}

int NTAPI SetStretchBltMode(void *dc, int mode) {
    (void)dc; (void)mode;
    return 1;
}

void *NTAPI GetStockObject(int obj) {
    (void)obj;
    static uint32_t stock = 0xBB000099;
    return &stock;
}

int NTAPI AlphaBlend(void *dc, int x, int y, int w, int h,
                     void *src, int sx, int sy, int sw, int sh,
                     void *blend) {
    (void)dc; (void)x; (void)y; (void)w; (void)h;
    (void)src; (void)sx; (void)sy; (void)sw; (void)sh; (void)blend;
    return 1;
}

/* ============================================================
 *  Stub Dispatch — Lookup functions by DLL + name
 * ============================================================ */

/* Define the stub entry table */
static const STUB_ENTRY g_stub_table[] = {
    /* ntdll.dll */
    {"ntdll.dll", "RtlAllocateHeap",               RtlAllocateHeap},
    {"ntdll.dll", "RtlFreeHeap",                   RtlFreeHeap},
    {"ntdll.dll", "RtlZeroMemory",                 RtlZeroMemory},
    {"ntdll.dll", "RtlCopyMemory",                 RtlCopyMemory},
    {"ntdll.dll", "RtlMoveMemory",                 RtlMoveMemory},
    {"ntdll.dll", "RtlCompareMemory",              RtlCompareMemory},
    {"ntdll.dll", "RtlFillMemory",                 RtlFillMemory},
    {"ntdll.dll", "RtlGetLastError",               RtlGetLastError},
    {"ntdll.dll", "RtlSetLastError",               RtlSetLastError},
    {"ntdll.dll", "RtlInitializeCriticalSection",  RtlInitializeCriticalSection},
    {"ntdll.dll", "RtlEnterCriticalSection",      RtlEnterCriticalSection},
    {"ntdll.dll", "RtlLeaveCriticalSection",      RtlLeaveCriticalSection},
    {"ntdll.dll", "RtlAcquirePebLock",            RtlAcquirePebLock},
    {"ntdll.dll", "RtlReleasePebLock",            RtlReleasePebLock},

    /* kernel32.dll */
    {"kernel32.dll", "GetModuleHandleA",            GetModuleHandleA},
    {"kernel32.dll", "GetProcAddress",              GetProcAddress},
    {"kernel32.dll", "LoadLibraryA",                LoadLibraryA},
    {"kernel32.dll", "LoadLibraryW",                LoadLibraryW},
    {"kernel32.dll", "FreeLibrary",                 FreeLibrary},
    {"kernel32.dll", "VirtualAlloc",                VirtualAlloc},
    {"kernel32.dll", "VirtualFree",                 VirtualFree},
    {"kernel32.dll", "HeapAlloc",                   HeapAlloc},
    {"kernel32.dll", "HeapFree",                    HeapFree},
    {"kernel32.dll", "GetProcessHeap",             GetProcessHeap},
    {"kernel32.dll", "Sleep",                       Sleep},
    {"kernel32.dll", "GetTickCount",               GetTickCount},
    {"kernel32.dll", "GetSystemInfo",              GetSystemInfo},
    {"kernel32.dll", "GetCommandLineA",            GetCommandLineA},
    {"kernel32.dll", "GetModuleHandleW",           GetModuleHandleW},
    {"kernel32.dll", "GetExitCodeProcess",        GetExitCodeProcess},
    {"kernel32.dll", "ExitProcess",                ExitProcess},
    {"kernel32.dll", "CreateThread",                CreateThread},
    {"kernel32.dll", "WaitForSingleObject",        WaitForSingleObject},
    {"kernel32.dll", "CloseHandle",                 CloseHandle},
    {"kernel32.dll", "GetLastError",               GetLastError},
    {"kernel32.dll", "SetLastError",               SetLastError},
    {"kernel32.dll", "OutputDebugStringA",         OutputDebugStringA},
    {"kernel32.dll", "OutputDebugStringW",         OutputDebugStringW},
    {"kernel32.dll", "GetModuleFileNameA",         GetModuleFileNameA},
    {"kernel32.dll", "LocalAlloc",                  LocalAlloc},
    {"kernel32.dll", "LocalFree",                   LocalFree},
    {"kernel32.dll", "QueryPerformanceCounter",    QueryPerformanceCounter},
    {"kernel32.dll", "QueryPerformanceFrequency",  QueryPerformanceFrequency},
    {"kernel32.dll", "GetCurrentProcess",          GetCurrentProcess},
    {"kernel32.dll", "GetCurrentThread",           GetCurrentThread},
    {"kernel32.dll", "GetCurrentProcessId",        GetCurrentProcessId},
    {"kernel32.dll", "GetCurrentThreadId",         GetCurrentThreadId},
    {"kernel32.dll", "IsProcessorFeaturePresent",  IsProcessorFeaturePresent},
    {"kernel32.dll", "InitializeCriticalSection",  InitializeCriticalSection},
    {"kernel32.dll", "EnterCriticalSection",      EnterCriticalSection},
    {"kernel32.dll", "LeaveCriticalSection",      LeaveCriticalSection},
    {"kernel32.dll", "InitializeCriticalSectionAndSpinCount", InitializeCriticalSectionAndSpinCount},
    {"kernel32.dll", "TryEnterCriticalSection",    TryEnterCriticalSection},
    {"kernel32.dll", "DeleteCriticalSection",      DeleteCriticalSection},
    {"kernel32.dll", "DisableThreadLibraryCalls",  DisableThreadLibraryCalls},
    {"kernel32.dll", "GetModuleHandleExA",         GetModuleHandleExA},
    {"kernel32.dll", "TlsGetValue",                TlsGetValue},
    {"kernel32.dll", "TlsSetValue",                TlsSetValue},
    {"kernel32.dll", "TlsAlloc",                   TlsAlloc},
    {"kernel32.dll", "TlsFree",                    TlsFree},
    {"kernel32.dll", "GetStartupInfoA",            GetStartupInfoA},

    /* user32.dll */
    {"user32.dll", "MessageBoxA",                  MessageBoxA},
    {"user32.dll", "MessageBoxW",                  MessageBoxW},
    {"user32.dll", "CreateWindowExA",              CreateWindowExA},
    {"user32.dll", "CreateWindowExW",              CreateWindowExW},
    {"user32.dll", "DestroyWindow",                DestroyWindow},
    {"user32.dll", "ShowWindow",                   ShowWindow},
    {"user32.dll", "UpdateWindow",                 UpdateWindow},
    {"user32.dll", "GetMessageA",                  GetMessageA},
    {"user32.dll", "PeekMessageA",                 PeekMessageA},
    {"user32.dll", "TranslateMessage",             TranslateMessage},
    {"user32.dll", "DispatchMessageA",             DispatchMessageA},
    {"user32.dll", "DefWindowProcA",               DefWindowProcA},
    {"user32.dll", "DefWindowProcW",               DefWindowProcW},
    {"user32.dll", "RegisterClassA",               RegisterClassA},
    {"user32.dll", "RegisterClassW",               RegisterClassW},
    {"user32.dll", "LoadCursorA",                  LoadCursorA},
    {"user32.dll", "LoadIconA",                    LoadIconA},
    {"user32.dll", "GetClientRect",                GetClientRect},
    {"user32.dll", "GetWindowRect",                GetWindowRect},
    {"user32.dll", "AdjustWindowRect",             AdjustWindowRect},
    {"user32.dll", "MoveWindow",                   MoveWindow},
    {"user32.dll", "SetWindowTextA",               SetWindowTextA},
    {"user32.dll", "GetWindowTextA",               GetWindowTextA},
    {"user32.dll", "GetWindowTextLengthA",         GetWindowTextLengthA},
    {"user32.dll", "InvalidateRect",               InvalidateRect},
    {"user32.dll", "BeginPaint",                   BeginPaint},
    {"user32.dll", "EndPaint",                     EndPaint},
    {"user32.dll", "GetDC",                        GetDC},
    {"user32.dll", "ReleaseDC",                    ReleaseDC},
    {"user32.dll", "PostQuitMessage",              PostQuitMessage},
    {"user32.dll", "SetFocus",                     SetFocus},
    {"user32.dll", "GetFocus",                     GetFocus},
    {"user32.dll", "IsWindow",                     IsWindow},
    {"user32.dll", "EnableWindow",                 EnableWindow},
    {"user32.dll", "SetTimer",                     SetTimer},
    {"user32.dll", "KillTimer",                    KillTimer},
    {"user32.dll", "DrawTextA",                    DrawTextA},
    {"user32.dll", "FillRect",                     FillRect},
    {"user32.dll", "SetBkColor",                   SetBkColor},
    {"user32.dll", "SetTextColor",                 SetTextColor},
    {"user32.dll", "MapVirtualKeyA",               MapVirtualKeyA},
    {"user32.dll", "GetKeyState",                  GetKeyState},
    {"user32.dll", "GetAsyncKeyState",             GetAsyncKeyState},
    {"user32.dll", "SetCapture",                   SetCapture},
    {"user32.dll", "ReleaseCapture",               ReleaseCapture},
    {"user32.dll", "GetCapture",                   GetCapture},
    {"user32.dll", "WindowFromPoint",             WindowFromPoint},
    {"user32.dll", "ChildWindowFromPoint",        ChildWindowFromPoint},
    {"user32.dll", "ScreenToClient",               ScreenToClient},
    {"user32.dll", "ClientToScreen",               ClientToScreen},
    {"user32.dll", "LoadBitmapA",                  LoadBitmapA},
    {"user32.dll", "LoadImageA",                   LoadImageA},
    {"user32.dll", "LoadCursorW",                  LoadCursorW},
    {"user32.dll", "LoadIconW",                    LoadIconW},
    {"user32.dll", "OpenClipboard",                OpenClipboard},
    {"user32.dll", "CloseClipboard",               CloseClipboard},
    {"user32.dll", "EmptyClipboard",               EmptyClipboard},
    {"user32.dll", "SetClipboardData",             SetClipboardData},
    {"user32.dll", "GetClipboardData",             GetClipboardData},

    /* gdi32.dll */
    {"gdi32.dll", "BitBlt",                        BitBlt},
    {"gdi32.dll", "StretchBlt",                    StretchBlt},
    {"gdi32.dll", "PatBlt",                        PatBlt},
    {"gdi32.dll", "CreateCompatibleDC",            CreateCompatibleDC},
    {"gdi32.dll", "DeleteDC",                      DeleteDC},
    {"gdi32.dll", "SelectObject",                  SelectObject},
    {"gdi32.dll", "DeleteObject",                  DeleteObject},
    {"gdi32.dll", "CreateCompatibleBitmap",        CreateCompatibleBitmap},
    {"gdi32.dll", "CreateSolidBrush",              CreateSolidBrush},
    {"gdi32.dll", "CreatePen",                     CreatePen},
    {"gdi32.dll", "CreateFontA",                   CreateFontA},
    {"gdi32.dll", "GetDeviceCaps",                 GetDeviceCaps},
    {"gdi32.dll", "Rectangle",                     Rectangle},
    {"gdi32.dll", "Ellipse",                       Ellipse},
    {"gdi32.dll", "MoveToEx",                      MoveToEx},
    {"gdi32.dll", "LineTo",                        LineTo},
    {"gdi32.dll", "SetPixel",                      SetPixel},
    {"gdi32.dll", "GetPixel",                      GetPixel},
    {"gdi32.dll", "TextOutA",                      TextOutA},
    {"gdi32.dll", "TextOutW",                      TextOutW},
    {"gdi32.dll", "SetBkMode",                     SetBkMode},
    {"gdi32.dll", "SetStretchBltMode",             SetStretchBltMode},
    {"gdi32.dll", "GetStockObject",                GetStockObject},
    {"gdi32.dll", "AlphaBlend",                    AlphaBlend},

    /* Terminator */
    {NULL, NULL, NULL}
};

/* -----------------------------------------------------------
 *  Win32StubsInit — Initialize the stub library.
 * ----------------------------------------------------------- */
void Win32StubsInit(void) {
    kputs("[Win32Stubs] Bibliotecas de stub carregadas (ntdll, kernel32, user32, gdi32)\n");
    kputs("[Win32Stubs] Total de stubs: ");
    {
        uint32_t count = 0;
        while (g_stub_table[count].func_ptr != NULL) ++count;
        char buf[8];
        buf[0] = '0' + (count / 100);
        buf[1] = '0' + ((count / 10) % 10);
        buf[2] = '0' + (count % 10);
        buf[3] = '\n';
        buf[4] = '\0';
        kputs(buf);
    }
}

/* -----------------------------------------------------------
 *  Win32StubResolve — Look up a function stub by DLL + name.
 *  This is the primary dispatch used by LdrResolveImport
 *  and LdrGetProcedureAddress.
 * ----------------------------------------------------------- */
void *Win32StubResolve(const char *dll_name, const char *func_name) {
    uint32_t i;

    if (!dll_name || !func_name) return NULL;

    for (i = 0; g_stub_table[i].func_ptr != NULL; ++i) {
        /* Compare DLL name (case-insensitive) */
        const char *a = g_stub_table[i].dll_name;
        const char *b = dll_name;
        int match = 1;
        while (*a && *b) {
            char ca = *a, cb = *b;
            if (ca >= 'A' && ca <= 'Z') ca += 0x20;
            if (cb >= 'A' && cb <= 'Z') cb += 0x20;
            if (ca != cb) { match = 0; break; }
            ++a; ++b;
        }
        if (!match) continue;

        /* Compare function name (case-sensitive, standard) */
        const char *fa = g_stub_table[i].func_name;
        const char *fb = func_name;
        match = 1;
        while (*fa && *fb) {
            if (*fa != *fb) { match = 0; break; }
            ++fa; ++fb;
        }
        if (match && *fa == '\0' && *fb == '\0') {
            return g_stub_table[i].func_ptr;
        }
    }

    return NULL; /* Not found */
}
