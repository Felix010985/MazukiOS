#include <stdint.h>
#include "kernel/api.h"
#include "kernel/serial.h"
#include "shell.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"


void kernel_main(void) {
    gdt_install();
    init_serial();
    puts_com1("COM1 Succesfully initialized!\n");
	shell_main();
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}
