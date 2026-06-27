#ifndef COREOS_DEBUG_CONSOLE_H
#define COREOS_DEBUG_CONSOLE_H

#include "coreos/types.h"

#define DBG_MAX_ARGS 16
#define DBG_LINE_BUF 256

/* Initialize the debug serial console */
void debug_console_init(void);

/* Run the interactive debug console loop (blocks) */
void debug_console_run(void);

/* Execute a single debug command (non-interactive) */
void debug_console_exec(const char *cmd_line);

#endif
