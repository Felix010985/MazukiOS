#include <stdint.h>
#include "kernel/api.h"
#include "kernel/serial.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "shell.h"

uint32_t user_stack[1024];
#define USER_STACK_TOP ((uint32_t)&user_stack[1024])

void jump_to_user(void (*user_entry)(void)) {
    asm volatile(
        "cli\n\t"                  // выключаем "вентиляторы"
        "mov $0x23, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        "pushl $0x23\n\t"
        "pushl %[stack_top]\n\t"
        "pushf\n\t"
        "pushl $0x1B\n\t"
        "pushl %[entry]\n\t"
        "iret\n\t"
        :
        : [stack_top]"r"(USER_STACK_TOP), [entry]"r"(user_entry)
        : "ax"
    );
}

void kernel_main(void) {
    gdt_install();
    idt_install();
    init_serial();
    puts_com1("COM1 Succesfully initialized!\n");
    jump_to_user(shell_main);
	// shell_main();
    //volatile uint16_t* vga = (uint16_t*)0xB8000;
	//char input[MAX_INPUT];
    //print("Hello People thats The Stacker of The Tesla\n", VGA_LIGHT_GRAY);
	//print("Enter a character: ", VGA_LIGHT_GRAY);
    //read_line(input, VGA_LIGHT_GREEN);
    //print("You entered: ", VGA_LIGHT_GRAY);
    //print(input, VGA_LIGHT_GRAY);
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}
