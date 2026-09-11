/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Masix Kernel
 * Copyright (C) 2026, FelixProfi. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */
#include <stdint.h>
#include <panic.h>
#include <serial.h>
#include <vga.h>
#include <bsod.h>

// extern void puts_com1(const char* s);

// struct exception_registers {
//     uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
//     uint32_t int_no, error_code;
//     uint32_t eip, cs, eflags;
// };

void bsod(const char* msg) {
    uint8_t bsod_color = 0x1F;
    vga_clear(bsod_color);

    print_bsod_str("A problem has been detected and Masix has been shut down to prevent damage\n");
    print_bsod_str("to your computer.\n\n");
    print_bsod_str("KERNEL_PANIC\n\n");
    print_bsod_str("If this is the first time you've seen this Stop error screen,\n");
    print_bsod_str("restart your computer. If this screen appears again, follow\n");
    print_bsod_str("these steps:\n\n");
    print_bsod_str("Check to make sure any new hardware is properly\n");
    print_bsod_str("installed. If this is a new installation, ask your hardware manufacturer\n");
    print_bsod_str("for any Masix updates you might need.\n\n");
    print_bsod_str("If problems continue, disable or remove any newly installed hardware\n");
    print_bsod_str("or software. Disable BIOS memory options such as caching or shadowing.\n\n");
    print_bsod_str("Technical Information:\n\n");
    print_bsod_str("*** STOP: 0x0000007B\n\n");
    print_bsod_str("*** kernel.org - Address 0xB00B1E55 base at Masix Core\n\n");

    print_bsod_str("Exception info: ");
    print_bsod_str(msg);
    print_bsod_str("\n");
}

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
    puts_com1("Masix: Panic: Calling vga_bsod(msg);");

    if (regs->int_no < 16) {
        bsod(exception_names[regs->int_no]);
    } else {
        bsod("Unknown Exception");
    }

    puts_com1("Masix: Panic: Maybe called, disabling interrupts...");
    __asm__ __volatile__("cli");

    puts_com1("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    puts_com1("!!!               KERNEL PANIC               !!!\n");

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

__attribute__((naked)) void exception_div_zero(void) {
    __asm__ __volatile__ (
        "pushl $0 \n\t"
        "pushl $0 \n\t"
        "jmp exception_common_stub \n\t"
    );
}

// void __stack_chk_fail(void) {
//     while(1) {
//         __asm__ __volatile__("cli; hlt");
//     }
// }
