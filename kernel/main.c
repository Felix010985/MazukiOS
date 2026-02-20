#include <stdint.h>
#include "kernel/api.h"
#include "kernel/serial.h"
#include "shell.h"


void kernel_main(void) {
    init_serial();
    puts_com1("COM1 Succesfully initialized!\n");
	shell_main();
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
