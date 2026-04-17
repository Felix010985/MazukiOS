#include "kernel/gdt.h"

struct gdt_entry gdt[5];
struct gdt_ptr gp;

void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = base & 0xFFFF;
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = limit & 0xFFFF;
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access      = access;
}

void gdt_install(void) {
    gp.limit = sizeof(gdt) - 1;
    gp.base  = (uint32_t)&gdt;

    gdt_set_entry(0, 0, 0, 0, 0);           // NULL
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel code
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel data
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User code (ring3)
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User data (ring3)

        asm volatile (
        "lgdt %0\n\t"
        "ljmp $0x08, $1f\n\t"
        "1:\n\t"
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        : : "m"(gp) : "ax", "memory"
    );
}
