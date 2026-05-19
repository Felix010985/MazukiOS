#include "kernel/api.h"
#include <stdint.h>

extern void idt_register_handler(uint8_t vector, uint32_t handler_addr, uint8_t flags);

extern void sys_print(const char* str, uint8_t color);
extern void sys_cls(void);
extern void sys_read_line(char* buffer, uint8_t color);

void syscall_handler_c(uint32_t syscall_num, uint32_t arg1, uint32_t arg2) {
    switch (syscall_num) {
        case 1:
            sys_print((const char*)arg1, (uint8_t)arg2);
            break;
        case 2:
            sys_cls();
            break;
        case 3:
            sys_read_line((char*)arg1, (uint8_t)arg2);
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

        "push %ecx \n\t"
        "push %ebx \n\t"
        "push %eax \n\t"
        "call syscall_handler_c \n\t"

        "add $12, %esp \n\t"
        "popa \n\t"
        "iret"
    );
}

void syscall_init(void) {
    idt_register_handler(0x80, (uint32_t)syscall_handler_asm, 0xEE);
}
