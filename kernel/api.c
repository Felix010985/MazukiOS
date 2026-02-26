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

// Очистка экрана
void cls(void) {
    uint16_t *terminal_buffer = (uint16_t *)0xB8000;
    uint16_t blank = (VGA_LIGHT_GRAY << 8) | ' ';

    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        terminal_buffer[i] = blank;
    }

    cursor_x = 0;
    cursor_y = 0;

    vga_update_cursor();
    cursor_x = 0;
    cursor_y = 0; // lowkirkuenly нихера не делает lmao
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
