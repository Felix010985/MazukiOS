#include "kernel/idt.h"

struct idt_entry idt[256];
struct idt_ptr idtp;

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;

    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

__attribute__((naked)) void default_interrupt_handler() {
    __asm__ __volatile__ (
        "cli\n\t"
        "hlt\n\t"
        "jmp ."
    );
}


void idt_install(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base  = (uint32_t)&idt;
    uint32_t handler_addr = (uint32_t)default_interrupt_handler;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, handler_addr, 0x08, 0x8E);
    }

    __asm__ __volatile__("lidt (%0)" : : "r"(&idtp));
}
// Должно работать
