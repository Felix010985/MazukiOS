#include "kernel/api.h"
#include "kernel/vga.h"
#include <stdarg.h>

void print_str(const char* s); // стринги
void print_num(int n);

static void putc_color(char c);

void printf(const char* fmt, ...);
int scanf(const char *fmt, ...);
