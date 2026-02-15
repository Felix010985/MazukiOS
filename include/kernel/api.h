#pragma once
#include <stdint.h>
#include <stddef.h>
#include "kernel/vga.h"

#define MAX_INPUT 128

void putc(char c);
void print(const char* str, uint8_t color);
void cls(void);
char getcin(void);
void read_line(char* buffer, uint8_t color);
int strcmp(const char* a, const char* b);

void* malloc(size_t size);
void free(void* ptr);

uint32_t k_get_ticks(void);
