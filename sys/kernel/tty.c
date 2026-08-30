#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 50

extern uint16_t cursor_x;
extern uint16_t cursor_y;
extern volatile uint16_t* vga_buffer;

extern void vga_raw_putc(char c, uint8_t color);
extern void vga_clear(uint8_t color);
extern void vga_update_cursor(void);
extern void vga_scroll(uint8_t color);

static uint8_t tty_color = 0x0F; // Текущий цвет терминала
static int ansi_state = 0;
static int ansi_arg1 = 0;
static int ansi_arg2 = 0;
static int* ansi_curr_arg = &ansi_arg1;

static const uint8_t ansi_to_vga_table[] = {0, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14, 9, 13, 11, 15};

// Честный деструктивный бэкспейс для TTY
void tty_backspace(void) {
    if (cursor_x > 0) {
        cursor_x--;
    } else if (cursor_y > 0) {
        cursor_y--;
        cursor_x = VGA_WIDTH - 1;
    }
    vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (tty_color << 8) | ' ';
    vga_update_cursor();
}

// Парсер ANSI последовательностей
static int parse_ansi(char c) {
    if (ansi_state == 0) {
        if (c == '\033') {
            ansi_state = 1;
            ansi_arg1 = 0; ansi_arg2 = 0;
            ansi_curr_arg = &ansi_arg1;
            return 1;
        }
        return 0;
    }
    if (ansi_state == 1) {
        if (c == '[') ansi_state = 2;
        else ansi_state = 0;
        return 1;
    }
    if (ansi_state == 2) {
        if (c >= '0' && c <= '9') {
            *ansi_curr_arg = (*ansi_curr_arg * 10) + (c - '0');
            return 1;
        }
        if (c == ';') {
            ansi_curr_arg = &ansi_arg2;
            return 1;
        }
        if (c == 'J' && ansi_arg1 == 2) {
            vga_clear(tty_color);
            ansi_state = 0;
            return 1;
        }
        if (c == 'H' || c == 'f') {
            cursor_x = 0; cursor_y = 0;
            vga_update_cursor();
            ansi_state = 0;
            return 1;
        }
        if (c == 'm') {
            if (ansi_arg1 == 0) tty_color = 0x0F;
            else if (ansi_arg1 >= 30 && ansi_arg1 <= 37) tty_color = (tty_color & 0xF0) | ansi_to_vga_table[ansi_arg1 - 30];
            else if (ansi_arg1 >= 90 && ansi_arg1 <= 97) tty_color = (tty_color & 0xF0) | ansi_to_vga_table[ansi_arg1 - 90];

            if (ansi_arg2 >= 30 && ansi_arg2 <= 37) tty_color = (tty_color & 0xF0) | ansi_to_vga_table[ansi_arg2 - 30];
            ansi_state = 0;
            return 1;
        }
        ansi_state = 0;
    }
    return 0;
}

// Главная POSIX функция TTY — принимает символ из сисколла write()
void tty_write_char(char c) {
    if (parse_ansi(c)) return;

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= VGA_HEIGHT) vga_scroll(tty_color);
        vga_update_cursor();
    } else if (c == '\r') {
        cursor_x = 0;
        vga_update_cursor();
    } else if (c == '\t') {
        cursor_x = (cursor_x + 8) & ~7;
        if (cursor_x >= VGA_WIDTH) { cursor_x = 0; cursor_y++; }
        if (cursor_y >= VGA_HEIGHT) vga_scroll(tty_color);
        vga_update_cursor();
    } else if (c == '\b') {
        tty_backspace();
    } else {
        vga_raw_putc(c, tty_color);
    }
}
