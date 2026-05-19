#include <stdint.h>
#include "kernel/panic.h"
#include "kernel/serial.h"

// extern void puts_com1(const char* s);

struct exception_registers {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, error_code;
    uint32_t eip, cs, eflags;
};

static const char* exception_names[] = {
    "Division By Zero", "Debug", "Non Maskable Interrupt", "Breakpoint",
    "Into Detected Overflow", "Out of Bounds", "Invalid Opcode", "No Coprocessor",
    "Double Fault", "Coprocessor Segment Overrun", "Bad TSS", "Segment Not Present",
    "Stack Fault", "General Protection Fault", "Page Fault", "Unknown Interrupt"
};

static void put_hex_com1(uint32_t val) {
    char hex_chars[] = "0123456789ABCDEF";
    char buf[11];
    buf[0] = '0'; buf[1] = 'x';
    for (int i = 0; i < 8; i++) {
        buf[9 - i] = hex_chars[(val >> (i * 4)) & 0x0F];
    }
    buf[10] = '\0';
    puts_com1(buf);
}

void kernel_exception_handler(struct exception_registers* regs) {
    __asm__ __volatile__("cli");

    puts_com1("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    puts_com1("!!!  KERNEL PANIC: CORE EXCEPTION CAUGHT  !!!\n");

    if (regs->int_no < 16) {
        puts_com1("Exception: "); puts_com1(exception_names[regs->int_no]); puts_com1("\n");
    } else {
        puts_com1("Exception: Unknown\n");
    }

    puts_com1("------------------------------------------------\n");
    puts_com1("EIP:    "); put_hex_com1(regs->eip);        puts_com1("   ERROR CODE: "); put_hex_com1(regs->error_code); puts_com1("\n");
    puts_com1("EAX:    "); put_hex_com1(regs->eax);        puts_com1("   EBX:        "); put_hex_com1(regs->ebx);        puts_com1("\n");
    puts_com1("ECX:    "); put_hex_com1(regs->ecx);        puts_com1("   EDX:        "); put_hex_com1(regs->edx);        puts_com1("\n");
    puts_com1("EDI:    "); put_hex_com1(regs->edi);        puts_com1("   ESI:        "); put_hex_com1(regs->esi);        puts_com1("\n");
    puts_com1("EBP:    "); put_hex_com1(regs->ebp);        puts_com1("   ESP:        "); put_hex_com1(regs->esp);        puts_com1("\n");
    puts_com1("CS:     "); put_hex_com1(regs->cs);         puts_com1("   EFLAGS:     "); put_hex_com1(regs->eflags);     puts_com1("\n");
    puts_com1("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

    for (;;) { __asm__ __volatile__("hlt"); }
}

void panic(const char* msg) {
    __asm__ __volatile__("cli");
    puts_com1("\n!!! KERNEL PANIC !!!\n");
    puts_com1(msg);
    puts_com1("\nSYSTEM HALTED\n");
    for (;;) { __asm__ __volatile__("hlt"); }
}

static __attribute__((naked)) void exception_common_stub(void) {
    __asm__ __volatile__ (
        "pusha \n\t"
        "mov $0x10, %ax \n\t"
        "mov %ax, %ds \n\t"
        "mov %ax, %es \n\t"

        "push %esp \n\t"
        "call kernel_exception_handler \n\t"
    );
}

__attribute__((naked)) void exception_gpf(void) {
    __asm__ __volatile__ (
        "pushl $13 \n\t"
        "jmp exception_common_stub \n\t"
    );
}
