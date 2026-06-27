#ifndef COREOS_ARCH_X86_SERIAL_H
#define COREOS_ARCH_X86_SERIAL_H

void serial_init(void);
void serial_putchar(char c);
void serial_puts(const char *s);
int serial_data_available(void);
char serial_getchar(void);
int serial_getchar_nonblock(char *c);
void serial_gets(char *buf, uint32_t max_len);

#endif
