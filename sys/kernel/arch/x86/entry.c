#include <stdint.h>

#define MULTIBOOT2_MAGIC 0xE85250D6
#define MULTIBOOT2_HEADER_LEN 16

__attribute__((section(".multiboot2"), used, aligned(8)))
const uint32_t multiboot2_header[] = {
    MULTIBOOT2_MAGIC,      // magic
    0,                     // architecture (0 = i386)
    MULTIBOOT2_HEADER_LEN, // header length
    -(MULTIBOOT2_MAGIC + 0 + MULTIBOOT2_HEADER_LEN),
    0, 8, 0                // end tag
};

extern void kernel_main(uint32_t magic, uint32_t addr);


__attribute__((naked)) void _start(void) {
    __asm__ __volatile__(
        "pushl %ebx \n\t"
        "pushl %eax \n\t"

        "call kernel_main \n\t"

        "1: \n\t"
        "hlt \n\t"
        "jmp 1b \n\t"
    );
}
