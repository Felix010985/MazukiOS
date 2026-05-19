#include "kernel/api.h"
#include <stdint.h>

extern void idt_register_handler(uint8_t vector, uint32_t handler_addr, uint8_t flags);

extern void sys_print(const char* str, uint8_t color);
extern void sys_cls(void);
extern void sys_read_line(char* buffer, uint8_t color);

struct syscall_regs {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
};

void syscall_handler_c(struct syscall_regs* regs) {
    switch (regs->eax) {
        case 1:
            sys_print((const char*)regs->ebx, (uint8_t)regs->ecx);
            break;
        case 2:
            sys_cls();
            break;
        case 3:
            sys_read_line((char*)regs->ebx, (uint8_t)regs->ecx);
            break;
        default:
            break;
    }
}

__attribute__((naked)) void syscall_handler_asm(void) {
    __asm__ __volatile__ (
        "pusha \n\t"

        "mov $0x10, %ax \n\t"
        "mov %ax, %ds \n\t"
        "mov %ax, %es \n\t"

        "sti \n\t"

        "push %esp \n\t"
        "call syscall_handler_c \n\t"
        "add $4, %esp \n\t"

        "cli \n\t"

        "mov $0x23, %ax \n\t"
        "mov %ax, %ds \n\t"
        "mov %ax, %es \n\t"

        "popa \n\t"
        "iret"
    );
}

void syscall_init(void) {
    idt_register_handler(0x80, (uint32_t)syscall_handler_asm, 0xEE);
}
