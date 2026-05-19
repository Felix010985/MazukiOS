#ifndef PANIC_H
#define PANIC_H
#pragma once

#include <stdint.h>

struct exception_registers {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, error_code;
    uint32_t eip, cs, eflags;
};
void kernel_exception_handler(struct exception_registers* regs);
void panic(const char* msg);

#endif
