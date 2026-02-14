#include <stdint.h>
#include "kernel/panic.h"

void panic(const char* msg) {
    volatile uint16_t* vga = (uint16_t*)0xB8000;
    int i = 0;

    while (msg[i]) {
        vga[i] = msg[i] | 0x4F00; // красный
        i++;
    }

    for (;;) {
        __asm__ __volatile__("hlt");
    }
}
