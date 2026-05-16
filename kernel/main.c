#include "kernel/gdt.h"
#include "kernel/io.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "kernel/idt.h"

void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);

// Выделяем два четких, раздельных стека
uint8_t user_stack[4096];
uint8_t kernel_stack[4096]; // Выделяем чистый стек специально для ядра!

extern void shell_main(void);
extern void init_serial(void);
extern void puts_com1(const char* s);
extern void keyboard_handler_c(void);
// Инициализация PIC8259, чтобы сдвинуть аппаратные прерывания на вектор 0x20
void pic_init(void) {
    outb(0x20, 0x11); outb(0xA0, 0x11); // ICW1: старт инициализации
    outb(0x21, 0x20); outb(0xA1, 0x28); // ICW2: базовые векторы (Мастер=0x20, Слейв=0x28)
    outb(0x21, 0x04); outb(0xA1, 0x02); // ICW3: каскадное соединение
    outb(0x21, 0x01); outb(0xA1, 0x01); // ICW4: режим 8086
    outb(0x21, 0xFD); outb(0xA1, 0xFF); // Маска: разрешаем ТОЛЬКО клавиатуру (IRQ1)
}

// Чистый ассемблерный обработчик для IDT
__attribute__((naked)) void keyboard_handler_asm(void) {
    __asm__ __volatile__ (
        "pusha \n\t"                  // Сохраняем все регистры

        "call keyboard_handler_c \n\t" // Твой рабочий драйвер печатает букву

        // Железно заносим 0x20 перед отправкой в порты PIC!
        "movb $0x20, %al \n\t"
        "outb %al, $0x20 \n\t"         // Сбрасываем Мастер-PIC
        "outb %al, $0xA0 \n\t"         // Сбрасываем Слейв-PIC

        "popa \n\t"                   // Восстанавливаем регистры
        "iret"                        // Возвращаемся в шелл крутить бесконечный ввод
    );
}

struct tss_entry_struct {
    uint32_t prev_tss; uint32_t esp0; uint32_t ss0; uint32_t esp1;
    uint32_t ss1; uint32_t esp2; uint32_t ss2; uint32_t cr3;
    uint32_t eip; uint32_t eflags; uint32_t eax; uint32_t ecx;
    uint32_t edx; uint32_t ebx; uint32_t esp; uint32_t ebp;
    uint32_t esi; uint32_t edi; uint32_t es; uint32_t cs;
    uint32_t ss; uint32_t ds; uint32_t fs; uint32_t gs;
    uint32_t ldt; uint16_t trap; uint16_t iomap_base;
} __attribute__((packed));

struct tss_entry_struct tss_entry;

void write_tss(int num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = sizeof(tss_entry) - 1;

    gdt_set_entry(num, base, limit, 0x89, 0x00);
    memset(&tss_entry, 0, sizeof(tss_entry));
    tss_entry.ss0 = ss0;
    tss_entry.esp0 = esp0;
}

void jump_to_user(void* shell_ptr, uint32_t user_esp) {
    asm volatile(
        "cli \n\t"
        "mov %0, %%ebx \n\t"
        "mov %1, %%edx \n\t"

        "mov $0x23, %%ax \n\t"
        "mov %%ax, %%ds \n\t"
        "mov %%ax, %%es \n\t"
        "mov %%ax, %%fs \n\t"
        "mov %%ax, %%gs \n\t"

        "pushl $0x23 \n\t"      // SS
        "pushl %%edx \n\t"      // ESP
        "pushfl \n\t"
        "popl %%eax \n\t"
        "orl $0x3200, %%eax \n\t" // IOPL (биты 12,13) + IF (бит 9)
        "pushl %%eax \n\t"
        "pushl $0x1B \n\t"      // CS
        "pushl %%ebx \n\t"      // EIP
        "iret"
        :
        : "r"(shell_ptr), "r"(user_esp)
        : "eax", "ebx", "edx", "memory"
    );
}

void kernel_main(void) {
    gdt_install();

    // 1. Устанавливаем чистую IDT со всеми дефолтными заглушками
    idt_install();

    // 2. Красиво и динамически регистрируем клавиатуру поверх заглушки
    extern void idt_register_handler(uint8_t vector, uint32_t handler_addr, uint8_t flags);
    extern void keyboard_handler_asm(void);

    // Вектор 0x21, адрес ассемблерной обертки, флаг 0xEE для юзермода
    idt_register_handler(0x21, (uint32_t)keyboard_handler_asm, 0x0EE);

    // 3. Инициализируем контроллер прерываний
    pic_init();

    write_tss(5, 0x10, (uint32_t)kernel_stack + 4096);
    asm volatile("ltr %%ax" : : "a"(0x2B));

    init_serial();
    puts_com1("COM1 Successfully initialized!\n");

    asm volatile("sti"); // Разрешаем прерывания процессору
    shell_main();
    //jump_to_user(&shell_main, (uint32_t)user_stack + 4096);

    for (;;) { asm volatile("hlt"); }
}

// ПОДСКАЗКИ ДЛЯ ДРУГИХ КТО БУДЕТ КОПАТЬ ЭТОТ КОД
