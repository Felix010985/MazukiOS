#include "kernel/gdt.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "kernel/gdt.h"
#include "kernel/idt.h"

void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

extern void shell_main(void);
extern void init_serial(void);
extern void puts_com1(const char* s);

// void* memset(void* dst, int value, unsigned int n) {
//     unsigned char* d = dst;
//     for (unsigned int i = 0; i < n; i++) d[i] = (unsigned char)value;
//     return dst;
// }


struct tss_entry_struct {
    uint32_t prev_tss; uint32_t esp0; uint32_t ss0; uint32_t esp1;
    uint32_t ss1; uint32_t esp2; uint32_t ss2; uint32_t cr3;
    uint32_t eip; uint32_t eflags; uint32_t eax; uint32_t ecx;
    uint32_t edx; uint32_t ebx; uint32_t esp; uint32_t ebp;
    uint32_t esi; uint32_t edi; uint32_t es; uint32_t cs;
    uint32_t ss; uint32_t ds; uint32_t fs; uint32_t gs;
    uint32_t ldt; uint16_t trap; uint16_t iomap_base;
} __attribute__((packed));

struct tss_entry_struct tss_entry;

// void *memset(void *dest, int val, uint32_t len) {
//     unsigned char *ptr = dest;
//     while (len-- > 0) *ptr++ = val;
//     return dest;
// }

uint32_t current_esp() {
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    return esp;
}

void write_tss(int num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = sizeof(tss_entry) - 1;

    gdt_set_entry(num, base, limit, 0x89, 0x00);
    memset(&tss_entry, 0, sizeof(tss_entry));
    tss_entry.ss0 = ss0;
    tss_entry.esp0 = esp0;
}

void jump_to_user(void* shell_ptr) {
    asm volatile(
        "mov $0x23, %%ax \n\t"
        "mov %%ax, %%ds \n\t"
        "mov %%ax, %%es \n\t"
        "mov %%ax, %%fs \n\t"
        "mov %%ax, %%gs \n\t"
        "mov %%esp, %%eax \n\t"
        "pushl $0x23 \n\t"      // SS
        "pushl %%eax \n\t"     // ESP
        "pushfl \n\t"
        "popl %%eax \n\t"
        "orl $0x200, %%eax \n\t" // Включаем прерывания (IF)
        "pushl %%eax \n\t"
        "pushl $0x1B \n\t"      // CS
        "pushl %0 \n\t"         // EIP
        "iret"
        : : "r"(shell_ptr) : "ax"
    );
}

void kernel_main(void) {
    gdt_install();

    write_tss(5, 0x10, current_esp());

    asm volatile("ltr %%ax" : : "a"(0x2B));

    init_serial();
    puts_com1("COM1 Succesfully initialized!\n");

    jump_to_user(&shell_main);

    for (;;) { asm volatile("hlt"); }
}
