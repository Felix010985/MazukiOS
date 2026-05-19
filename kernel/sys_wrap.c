#include "kernel/api.h"

void print(const char* str, uint8_t color) {
    __asm__ __volatile__ (
        "movl $1, %%eax \n\t"
        "movl %0, %%ebx \n\t"
        "movl %1, %%ecx \n\t"
        "int $0x80"
        :
        : "r"(str), "r"((uint32_t)color)
        : "eax", "ebx", "ecx"
    );
}

void cls(void) {
    __asm__ __volatile__ (
        "movl $2, %%eax \n\t"
        "int $0x80"
        :
        :
        : "eax"
    );
}

void read_line(char* buffer, uint8_t color) {
    __asm__ __volatile__ (
        "movl $3, %%eax \n\t"
        "movl %0, %%ebx \n\t"
        "movl %1, %%ecx \n\t"
        "int $0x80"
        :
        : "r"(buffer), "r"((uint32_t)color)
        : "eax", "ebx", "ecx"
    );
}
