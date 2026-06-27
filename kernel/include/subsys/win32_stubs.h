#ifndef COREOS_SUBSYS_WIN32_STUBS_H
#define COREOS_SUBSYS_WIN32_STUBS_H

#include "nt/ntdef.h"
#include "nt/ntstatus.h"

/* ============================================================
 *  Win32 Stub DLL Functions
 *
 *  Built-in implementations of common Win32 API functions.
 *  These are called when a loaded PE module imports from
 *  kernel32.dll, user32.dll, gdi32.dll, or ntdll.dll.
 *
 *  Each stub logs its call and returns a reasonable default.
 * ============================================================ */

/* ---- Initialize/resolve ---- */
void  Win32StubsInit(void);
void *Win32StubResolve(const char *dll_name, const char *func_name);

/* ============================================================
 *  NTDLL Stubs (ntdll.dll)
 * ============================================================ */
void *NTAPI RtlAllocateHeap(void *heap, uint32_t flags, uint32_t size);
int   NTAPI RtlFreeHeap(void *heap, uint32_t flags, void *ptr);
void  NTAPI RtlZeroMemory(void *ptr, uint32_t size);
void  NTAPI RtlCopyMemory(void *dst, const void *src, uint32_t size);
void  NTAPI RtlMoveMemory(void *dst, const void *src, uint32_t size);
int   NTAPI RtlCompareMemory(const void *a, const void *b, uint32_t size);
void  NTAPI RtlFillMemory(void *ptr, uint32_t size, uint8_t fill);
uint32_t NTAPI RtlGetLastError(void);
void  NTAPI RtlSetLastError(uint32_t error);
int   NTAPI RtlInitializeCriticalSection(void *cs);
int   NTAPI RtlEnterCriticalSection(void *cs);
int   NTAPI RtlLeaveCriticalSection(void *cs);
void  NTAPI RtlAcquirePebLock(void);
void  NTAPI RtlReleasePebLock(void);

/* ============================================================
 *  KERNEL32 Stubs (kernel32.dll)
 * ============================================================ */
void *NTAPI GetModuleHandleA(const char *module_name);
void *NTAPI GetProcAddress(void *module, const char *name);
void *NTAPI LoadLibraryA(const char *lib_name);
int   NTAPI FreeLibrary(void *module);
void *NTAPI VirtualAlloc(void *addr, uint32_t size, uint32_t type, uint32_t prot);
int   NTAPI VirtualFree(void *addr, uint32_t size, uint32_t type);
void *NTAPI HeapAlloc(void *heap, uint32_t flags, uint32_t size);
int   NTAPI HeapFree(void *heap, uint32_t flags, void *ptr);
void *NTAPI GetProcessHeap(void);
void  NTAPI Sleep(uint32_t ms);
uint32_t NTAPI GetTickCount(void);
int   NTAPI GetSystemInfo(void *info);
void *NTAPI GetCommandLineA(void);
void *NTAPI GetModuleHandleW(const void *name);
void *NTAPI LoadLibraryW(const void *name);
int   NTAPI GetExitCodeProcess(void *process, uint32_t *code);
void  NTAPI ExitProcess(uint32_t code);
void *NTAPI CreateThread(void *attr, uint32_t stack, void *start, void *param,
                         uint32_t flags, uint32_t *id);
uint32_t NTAPI WaitForSingleObject(void *handle, uint32_t ms);
int   NTAPI CloseHandle(void *handle);
uint32_t NTAPI GetLastError(void);
void  NTAPI SetLastError(uint32_t error);
void  NTAPI OutputDebugStringA(const char *str);
void  NTAPI OutputDebugStringW(const void *str);
uint32_t NTAPI GetModuleFileNameA(void *module, char *buf, uint32_t size);
void *NTAPI LocalAlloc(uint32_t flags, uint32_t size);
void *NTAPI LocalFree(void *ptr);
int   NTAPI QueryPerformanceCounter(void *counter);
int   NTAPI QueryPerformanceFrequency(void *freq);
void *NTAPI GetCurrentProcess(void);
void *NTAPI GetCurrentThread(void);
uint32_t NTAPI GetCurrentProcessId(void);
uint32_t NTAPI GetCurrentThreadId(void);
int   NTAPI IsProcessorFeaturePresent(uint32_t feature);
void  NTAPI InitializeCriticalSection(void *cs);
void  NTAPI EnterCriticalSection(void *cs);
void  NTAPI LeaveCriticalSection(void *cs);
int   NTAPI InitializeCriticalSectionAndSpinCount(void *cs, uint32_t count);
int   NTAPI TryEnterCriticalSection(void *cs);
void  NTAPI DeleteCriticalSection(void *cs);
void *NTAPI GetModuleHandleExA(uint32_t flags, const char *name, void **module);
int   NTAPI DisableThreadLibraryCalls(void *module);
void *NTAPI TlsGetValue(uint32_t index);
int   NTAPI TlsSetValue(uint32_t index, void *value);
uint32_t NTAPI TlsAlloc(void);
int   NTAPI TlsFree(uint32_t index);
void *NTAPI GetStartupInfoA(void *info);

/* ============================================================
 *  USER32 Stubs (user32.dll)
 * ============================================================ */
int   NTAPI MessageBoxA(void *hwnd, const char *text, const char *caption, uint32_t type);
int   NTAPI MessageBoxW(void *hwnd, const void *text, const void *caption, uint32_t type);
void *NTAPI CreateWindowExA(uint32_t ex, const char *cls, const char *title,
                            uint32_t style, int x, int y, int w, int h,
                            void *parent, void *menu, void *inst, void *param);
void *NTAPI CreateWindowExW(uint32_t ex, const void *cls, const void *title,
                            uint32_t style, int x, int y, int w, int h,
                            void *parent, void *menu, void *inst, void *param);
int   NTAPI DestroyWindow(void *hwnd);
int   NTAPI ShowWindow(void *hwnd, int cmd);
int   NTAPI UpdateWindow(void *hwnd);
int   NTAPI GetMessageA(void *msg, void *hwnd, uint32_t min, uint32_t max);
int   NTAPI PeekMessageA(void *msg, void *hwnd, uint32_t min, uint32_t max, uint32_t remove);
int   NTAPI TranslateMessage(const void *msg);
int   NTAPI DispatchMessageA(const void *msg);
void *NTAPI DefWindowProcA(void *hwnd, uint32_t msg, uint32_t wp, uint32_t lp);
void *NTAPI DefWindowProcW(void *hwnd, uint32_t msg, uint32_t wp, uint32_t lp);
void *NTAPI RegisterClassA(const void *cls);
void *NTAPI RegisterClassW(const void *cls);
void *NTAPI LoadCursorA(void *inst, const char *name);
void *NTAPI LoadIconA(void *inst, const char *name);
int   NTAPI GetClientRect(void *hwnd, void *rect);
int   NTAPI GetWindowRect(void *hwnd, void *rect);
int   NTAPI AdjustWindowRect(void *rect, uint32_t style, int menu);
int   NTAPI MoveWindow(void *hwnd, int x, int y, int w, int h, int repaint);
int   NTAPI SetWindowTextA(void *hwnd, const char *text);
int   NTAPI GetWindowTextA(void *hwnd, char *buf, int max);
uint32_t NTAPI GetWindowTextLengthA(void *hwnd);
int   NTAPI InvalidateRect(void *hwnd, const void *rect, int erase);
void *NTAPI BeginPaint(void *hwnd, void *ps);
int   NTAPI EndPaint(void *hwnd, const void *ps);
void *NTAPI GetDC(void *hwnd);
int   NTAPI ReleaseDC(void *hwnd, void *dc);
int   NTAPI PostQuitMessage(int code);
void *NTAPI SetFocus(void *hwnd);
void *NTAPI GetFocus(void);
int   NTAPI IsWindow(void *hwnd);
int   NTAPI EnableWindow(void *hwnd, int enable);
int   NTAPI SetTimer(void *hwnd, uint32_t id, uint32_t ms, void *proc);
int   NTAPI KillTimer(void *hwnd, uint32_t id);
int   NTAPI DrawTextA(void *dc, const char *text, int count, void *rect, uint32_t fmt);
int   NTAPI FillRect(void *dc, const void *rect, void *brush);
int   NTAPI SetBkColor(void *dc, uint32_t color);
int   NTAPI SetTextColor(void *dc, uint32_t color);
uint32_t NTAPI MapVirtualKeyA(uint32_t code, uint32_t type);
int   NTAPI GetKeyState(int key);
int   NTAPI GetAsyncKeyState(int key);
void *NTAPI SetCapture(void *hwnd);
int   NTAPI ReleaseCapture(void);
void *NTAPI GetCapture(void);
void *NTAPI WindowFromPoint(int x, int y);
void *NTAPI ChildWindowFromPoint(void *parent, int x, int y);
int   NTAPI ScreenToClient(void *hwnd, void *point);
int   NTAPI ClientToScreen(void *hwnd, void *point);
void *NTAPI LoadBitmapA(void *inst, const char *name);
void *NTAPI LoadImageA(void *inst, const char *name, uint32_t type,
                        int cx, int cy, uint32_t flags);
void *NTAPI LoadCursorW(void *inst, const void *name);
void *NTAPI LoadIconW(void *inst, const void *name);
int   NTAPI OpenClipboard(void *hwnd);
int   NTAPI CloseClipboard(void);
int   NTAPI EmptyClipboard(void);
void *NTAPI SetClipboardData(uint32_t fmt, void *data);
void *NTAPI GetClipboardData(uint32_t fmt);

/* ============================================================
 *  GDI32 Stubs (gdi32.dll)
 * ============================================================ */
int   NTAPI BitBlt(void *dc, int x, int y, int w, int h,
                   void *src_dc, int src_x, int src_y, uint32_t op);
int   NTAPI StretchBlt(void *dc, int x, int y, int w, int h,
                       void *src_dc, int sx, int sy, int sw, int sh, uint32_t op);
int   NTAPI PatBlt(void *dc, int x, int y, int w, int h, uint32_t op);
void *NTAPI CreateCompatibleDC(void *dc);
int   NTAPI DeleteDC(void *dc);
void *NTAPI SelectObject(void *dc, void *obj);
int   NTAPI DeleteObject(void *obj);
void *NTAPI CreateCompatibleBitmap(void *dc, int w, int h);
void *NTAPI CreateSolidBrush(uint32_t color);
void *NTAPI CreatePen(int style, int width, uint32_t color);
void *NTAPI CreateFontA(int h, int w, int esc, int orient, uint32_t weight,
                        uint32_t italic, uint32_t uline, uint32_t strike,
                        uint32_t charset, uint32_t out, uint32_t clip,
                        uint32_t qual, const char *face);
int   NTAPI GetDeviceCaps(void *dc, int index);
int   NTAPI Rectangle(void *dc, int left, int top, int right, int bottom);
int   NTAPI Ellipse(void *dc, int left, int top, int right, int bottom);
int   NTAPI MoveToEx(void *dc, int x, int y, void *old);
int   NTAPI LineTo(void *dc, int x, int y);
int   NTAPI SetPixel(void *dc, int x, int y, uint32_t color);
uint32_t NTAPI GetPixel(void *dc, int x, int y);
int   NTAPI TextOutA(void *dc, int x, int y, const char *text, int count);
int   NTAPI TextOutW(void *dc, int x, int y, const void *text, int count);
int   NTAPI SetBkMode(void *dc, int mode);
int   NTAPI SetStretchBltMode(void *dc, int mode);
void *NTAPI GetStockObject(int obj);
int   NTAPI AlphaBlend(void *dc, int x, int y, int w, int h,
                       void *src, int sx, int sy, int sw, int sh,
                       void *blend);

#endif /* COREOS_SUBSYS_WIN32_STUBS_H */
