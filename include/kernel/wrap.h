#ifndef USER_API_H
#define USER_API_H

#include <stdint.h>
#include <stddef.h>

#define MAX_INPUT 128

void print(const char* str, uint8_t color);
void cls(void);
void read_line(char* buffer, uint8_t color);

int strcmp(const char* a, const char* b);

#endif
