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

void cls(void) {
    // Сохраняем координаты prompt
    int saved_x = cursor_x;
    int saved_y = cursor_y;

    // Используем временные координаты для очистки
    int temp_cursor_x = 0;
    int temp_cursor_y = 0;

    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            // Меняем глобальные курсоры на временные перед вызовом vga_putc_color
            int old_x = cursor_x;
            int old_y = cursor_y;
            cursor_x = temp_cursor_x;
            cursor_y = temp_cursor_y;

            vga_putc_color(' ', VGA_LIGHT_GRAY);

            temp_cursor_x++;
            if (temp_cursor_x >= VGA_WIDTH) {
                temp_cursor_x = 0;
                temp_cursor_y++;
            }

            // Восстанавливаем глобальные координаты после вывода символа
            cursor_x = old_x;
            cursor_y = old_y;
        }
    }

    // Восстанавливаем курсор на prompt
    cursor_x = saved_x;
    cursor_y = saved_y;
    vga_update_cursor();
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
