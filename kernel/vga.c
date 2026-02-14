#include <stdint.h>

static uint16_t cursor_x = 0;
static uint16_t cursor_y = 0;
#define VGA_WIDTH 80
#define VGA_HEIGHT 25


volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;
uint16_t vga_index = 0;  // текущая позиция в буфере

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void vga_update_cursor(void) {
    uint16_t pos = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}


void vga_putc(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (0x0F << 8) | c;
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        for (int y = 1; y < VGA_HEIGHT; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                vga_buffer[(y-1) * VGA_WIDTH + x] = vga_buffer[y * VGA_WIDTH + x];
            }
        }
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(VGA_HEIGHT-1) * VGA_WIDTH + x] = (0x0F << 8) | ' ';
        }
        cursor_y = VGA_HEIGHT - 1;
    }

    vga_update_cursor();
}

void vga_putc_color(char c, uint8_t color) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0)
            cursor_x--;
        else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = VGA_WIDTH - 1;
        }
        // стираем символ пробелом с тем же цветом, что был на экране
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (0x07 << 8) | ' '; // курсор “серый”
        vga_update_cursor();
        return;
    } else {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | c;
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    if (cursor_y >= VGA_HEIGHT) {
        // scroll
        for (int y = 1; y < VGA_HEIGHT; y++)
            for (int x = 0; x < VGA_WIDTH; x++)
                vga_buffer[(y-1) * VGA_WIDTH + x] = vga_buffer[y * VGA_WIDTH + x];

        for (int x = 0; x < VGA_WIDTH; x++)
            vga_buffer[(VGA_HEIGHT-1) * VGA_WIDTH + x] = (0x0F << 8) | ' ';
        cursor_y = VGA_HEIGHT - 1;
    }

    vga_update_cursor();
}
