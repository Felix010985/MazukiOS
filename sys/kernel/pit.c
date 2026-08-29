#include "kernel/pit.h"
#include "kernel/io.h"
#include <stdint.h>

static volatile uint32_t system_ticks = 0;

extern void idt_register_handler(uint8_t vector, uint32_t handler_addr, uint8_t flags);

void pit_handler_c(void) {
    system_ticks++;
}

__attribute__((naked)) void pit_handler_asm(void) {
    __asm__ __volatile__ (
        "pusha \n\t"
        "mov $0x10, %ax \n\t"
        "mov %ax, %ds \n\t"
        "mov %ax, %es \n\t"

        "call pit_handler_c \n\t"

        "mov $0x20, %al \n\t"
        "outb %al, $0x20 \n\t"

        "popa \n\t"
        "iret"
    );
}

void pit_init(uint32_t frequency) {
    uint32_t divisor = PIT_BASE_FREQUENCY / frequency;

    outb(PIT_PORT_COMMAND, 0x36);

    outb(PIT_PORT_DATA0, (uint8_t)(divisor & 0xFF));
    outb(PIT_PORT_DATA0, (uint8_t)((divisor >> 8) & 0xFF));

    idt_register_handler(0x20, (uint32_t)pit_handler_asm, 0x8E);
}

uint32_t pit_get_ticks(void) {
    return system_ticks;
}

void sleep(uint32_t ticks) {
    uint32_t target_ticks = system_ticks + ticks;
    while (system_ticks < target_ticks) {
    }
}
