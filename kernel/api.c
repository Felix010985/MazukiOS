#include "kernel/api.h"
#include "kernel/vga.h"
#include "kernel/keyboard.h"
#include "kernel/alloc.h"

#define MAX_INPUT 128

// Вывод символа
void putc(char c) {
    vga_putc(c);
}

// Вывод строки
void print(const char* str, uint8_t color) {
    while (*str) {
        vga_putc_color(*str++, color);
    }
}

// Ввод символа с клавиатуры
char getcin(void) {
    return keyboard_getc();
}

// Чтение строки с клавиатуры (макс. 128 символов)
void read_line(char* buffer, uint8_t color) {
    int i = 0;

    while (1) {
        char c = keyboard_getc();

        if (c == '\n') {
            buffer[i] = 0;
            vga_putc_color('\n', color);
            return;
        } else if (c == '\b') {
            if (i > 0) {
                i--;
                vga_putc_color('\b', color);
                vga_putc_color(' ', color);
                vga_putc_color('\b', color);
            }
        } else {
            if (i < MAX_INPUT - 1) {
                buffer[i++] = c;
                vga_putc_color(c, color);
            }
        }
    }
}

// Выделение памяти
void* malloc(size_t size) {
    return alloc(size);
}

// Освобождение памяти
void free(void* ptr) {
    alloc_free(ptr);
}
