ENTRY(_start)

SECTIONS {
    . = 1M;  /* GRUB требует >=1MB */

    .multiboot2 : { KEEP(*(.multiboot2)) }

    .text : ALIGN(4K) {
        *(.text)
    }

    .data : ALIGN(4K) {
        *(.data)
        *(.rodata)
    }

    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss)
    }
}
