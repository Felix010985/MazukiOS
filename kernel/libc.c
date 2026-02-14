#include "libc.h"
#include "kernel/api.h"
#include "kernel/vga.h"
#include <stdarg.h>

void* memcpy(void* dst, const void* src, unsigned int n) {
    unsigned char* d = dst;
    const unsigned char* s = src;
    for (unsigned int i = 0; i < n; i++) d[i] = s[i];
    return dst;
}

void* memset(void* dst, int value, unsigned int n) {
    unsigned char* d = dst;
    for (unsigned int i = 0; i < n; i++) d[i] = (unsigned char)value;
    return dst;
}

unsigned int strlen(const char* s) {
    unsigned int i = 0;
    while (s[i]) i++;
    return i;
}

char* strcpy(char* dst, const char* src) {
    unsigned int i = 0;
    while (src[i]) { dst[i] = src[i]; i++; }
    dst[i] = 0;
    return dst;
}

int strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) {
        a++;
        b++;
    }
    return (unsigned char)(*a) - (unsigned char)(*b);
}

// int -> string (base 10 или 16)
void itoa(int value, char* str, int base) {
    char buf[33];
    int i = 0, is_negative = 0;
    if (value == 0) { str[0]='0'; str[1]=0; return; }
    if (base == 10 && value < 0) { is_negative = 1; value = -value; }
    while (value) {
        int rem = value % base;
        buf[i++] = (rem < 10) ? ('0'+rem) : ('A'+rem-10);
        value /= base;
    }
    if (is_negative) buf[i++] = '-';
    int j=0;
    while (i>0) str[j++] = buf[--i];
    str[j]=0;
}

int atoi(const char* str) {
    int res=0, sign=1, i=0;
    if (str[0]=='-') { sign=-1; i=1; }
    for (; str[i]; i++) res = res*10 + (str[i]-'0');
    return res*sign;
}

void print_str(const char* s) {
    int i=0;
    while(s[i]) { print(&s[i], VGA_LIGHT_GRAY); i++; }
}

void print_num(int n) {
    char buf[16];
    itoa(n, buf, 10);
    print_str(buf);
}

static int current_color = VGA_LIGHT_GRAY;

void set_color(int color) {
    current_color = color;
}

void reset_color() {
    current_color = VGA_LIGHT_GRAY;
}

static void putc_color(char c) {
    char tmp[2];
    tmp[0] = c;
    tmp[1] = 0;
    print(tmp, current_color);
}

void printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    for (int i = 0; fmt[i]; i++) {
        if (fmt[i] == '%' && fmt[i+1]) {
            i++;
            if (fmt[i] == 's') {
                const char* s = va_arg(args, const char*);
                for (int j = 0; s[j]; j++)
                    putc_color(s[j]);
            } else if (fmt[i] == 'd') {
                int num = va_arg(args, int);
                char buf[32];
                itoa(num, buf, 10);
                for (int j = 0; buf[j]; j++)
                    putc_color(buf[j]);
            } else if (fmt[i] == 'x') {
                int num = va_arg(args, int);
                char buf[32];
                itoa(num, buf, 16);
                for (int j = 0; buf[j]; j++)
                    putc_color(buf[j]);
            } else if (fmt[i] == 'c') {
                char c = (char)va_arg(args, int);
                putc_color(c);
            } else if (fmt[i] == '%') {
                putc_color('%');
            }
        } else {
            putc_color(fmt[i]);
        }
    }

    va_end(args);
}
