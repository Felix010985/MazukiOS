#ifndef LIBC_H
#define LIBC_H

void set_color(int color);
void reset_color(void);
void print_str(const char* s);
void print_num(int n);
void printf(const char* fmt, ...);

void* memcpy(void* dst, const void* src, unsigned int n);
void* memset(void* dst, int value, unsigned int n);
unsigned int strlen(const char* s);
char* strcpy(char* dst, const char* src);
int strcmp(const char* a, const char* b);
void itoa(int value, char* str, int base);
int atoi(const char* str);

#endif
