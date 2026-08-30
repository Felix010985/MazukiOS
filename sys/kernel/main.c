#include "kernel/gdt.h"
#include "kernel/io.h"
#include "kernel/idt.h"
#include "kernel/pit.h"
#include "kernel/panic.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

uint32_t shell_elf_start = 0;
uint32_t shell_elf_size = 0;

extern void printf(const char* fmt, ...);
extern void tty_write_char(char c);

void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
extern void pic_init(void);
extern void keyboard_init(void);

extern void init_serial(void);
extern void puts_com1(const char* s);

extern void exception_gpf(void);
extern void idt_register_handler(uint8_t vector, uint32_t handler_addr, uint8_t flags);

uint8_t user_stack[4096];
uint8_t kernel_stack[4096];

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_tag_module {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char string;
};

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

void write_tss(int num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = sizeof(tss_entry) - 1;

    memset(&tss_entry, 0, sizeof(tss_entry));

    tss_entry.ss0 = ss0;
    tss_entry.esp0 = esp0;

    tss_entry.iomap_base = sizeof(tss_entry);

    gdt_set_entry(num, base, limit, 0x89, 0x00);
}

__attribute__((naked)) void jump_to_user(void* shell_ptr, uint32_t user_esp) {
    __asm__ __volatile__(
        "cli \n\t"
        "movl 4(%%esp), %%ebx \n\t"
        "movl 8(%%esp), %%edx \n\t"

        "mov $0x23, %%ax \n\t"
        "mov %%ax, %%ds \n\t"
        "mov %%ax, %%es \n\t"
        "mov %%ax, %%fs \n\t"
        "mov %%ax, %%gs \n\t"
        "pushl $0x23 \n\t"
        "pushl %%edx \n\t"
        "pushl $0x0202 \n\t"
        "pushl $0x1B \n\t"
        "pushl %%ebx \n\t"

        "iret \n\t"
        :
        :
        : "eax", "ebx", "edx", "memory"
    );
}

void kernel_main(uint32_t magic, uint32_t addr) {
    gdt_install();
    idt_install();

    uint32_t cr0;
    uint32_t cr4;


    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 2); // Сбросить EM (Bit 2)
    cr0 |= (1 << 1);  // Установить MP (Bit 1)
    asm volatile("mov %0, %%cr0" : : "r"(cr0));

    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1 << 9);
    cr4 |= (1 << 10);
    asm volatile("mov %0, %%cr4" : : "r"(cr4));

    extern void exception_gpf(void);
    idt_register_handler(13, (uint32_t)exception_gpf, 0x8E);
    extern void exception_div_zero(void);
    idt_register_handler(0, (uint32_t)exception_div_zero, 0x8E);

    pic_init();
    keyboard_init();
    // pit_init(100);

    extern void syscall_init(void);
    syscall_init();

    write_tss(5, 0x10, (uint32_t)kernel_stack + 4096);
    asm volatile("ltr %%ax" : : "a"(0x28));

    init_serial();
    puts_com1("COM1 Successfully initialized!\n");

    // asm volatile("sti");
    puts_com1("BEFORE JUMP\n");

    if (magic == 0x36d76289 && addr != 0) {
        uint8_t* tag_ptr = (uint8_t*)(addr + 8);

        while (1) {
            struct multiboot_tag* tag = (struct multiboot_tag*)tag_ptr;

            if (tag->type == 0) break;

            if (tag->type == 3) {
                struct multiboot_tag_module* mod = (struct multiboot_tag_module*)tag;
                shell_elf_start = mod->mod_start;
                shell_elf_size = mod->mod_end - mod->mod_start;
                break;
            }

            tag_ptr += ((tag->size + 7) & ~7);
        }
    }

    extern void task_init(void);
    task_init();
    puts_com1("Masix: Task manager initialized.\n");

    if (shell_elf_start != 0 && shell_elf_size != 0) {
        extern void* elf_load_binary(uint32_t file_start);
        void* entry_point = elf_load_binary(shell_elf_start);

        if (entry_point != NULL) {
            puts_com1("Masix: Creating userland task for shell.elf...\n");

            extern void task_create(void* entry_point);
            task_create(entry_point);

        } else {
            puts_com1("CRITICAL: ELF binary loading failed! Halted.\n");
            for (;;) { asm volatile("hlt"); }
        }
    } else {
        puts_com1("CRITICAL: shell.elf module not found in multiboot tags! Halted.\n");
        for (;;) { asm volatile("hlt"); }
    }

    pit_init(100);
    puts_com1("Masix: PIT Timer registered at 100Hz.\n");

    puts_com1("Masix: Multitasking started! Jumping to Ring 3...\n");

    asm volatile("sti");

    for (;;) { asm volatile("hlt"); }
}
