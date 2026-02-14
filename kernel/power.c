#include <stdint.h>
#include "kernel/api.h"

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

void system_shutdown(void) {
    outw(0x604, 0x2000);

    print("System halted\n", VGA_RED);
    __asm__ volatile("cli");
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

void shutdown_by_panic() {
    __asm__ volatile ("cli");
    uint64_t idt_ptr = 0;
    __asm__ volatile ("lidt (%0)" : : "r"(&idt_ptr));
    __asm__ volatile ("int3");
}

