#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

uint16_t cursor_x = 0;
uint16_t cursor_y = 0;
volatile uint16_t* vga_buffer = (volatile uint16_t*)0xB8000;

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

void vga_scroll(uint8_t color) {
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(y-1) * VGA_WIDTH + x] = vga_buffer[y * VGA_WIDTH + x];
        }
    }
    uint16_t blank = (color << 8) | ' ';
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT-1) * VGA_WIDTH + x] = blank;
    }
    cursor_y = VGA_HEIGHT - 1;
}

void vga_clear(uint8_t color) {
    uint16_t blank = (color << 8) | ' ';
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = blank;
    }
    cursor_x = 0;
    cursor_y = 0;
    vga_update_cursor();
}

void vga_raw_putc(char c, uint8_t color) {
    vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = (color << 8) | c;
    cursor_x++;

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= VGA_HEIGHT) {
        vga_scroll(color);
    }
    vga_update_cursor();
}

void vga_putc(char c) {
    vga_raw_putc(c, 0x0F);
}
// вспомогательная функция для вывода BSOD сообщения
static void print_bsod_str(const char* s) {
    uint8_t bsod_color = 0x1F;
    while (*s) {
        if (*s == '\n') {
            cursor_x = 0;
            cursor_y++;
            if (cursor_y >= VGA_HEIGHT) vga_scroll(bsod_color);
            vga_update_cursor();
            s++;
            continue;
        }
        vga_raw_putc(*s++, bsod_color);
    }
}

void vga_bsod(const char* msg) {
    uint8_t bsod_color = 0x1F;
    vga_clear(bsod_color);

    print_bsod_str("A problem has been detected and Masix has been shut down to prevent damage\n");
    print_bsod_str("to your computer.\n\n");
    print_bsod_str("KERNEL_PANIC\n\n");
    print_bsod_str("If this is the first time you've seen this Stop error screen,\n");
    print_bsod_str("restart your computer. If this screen appears again, follow\n");
    print_bsod_str("these steps:\n\n");
    print_bsod_str("Check to make sure any new hardware is properly\n");
    print_bsod_str("installed. If this is a new installation, ask your hardware manufacturer\n");
    print_bsod_str("for any Masix updates you might need.\n\n");
    print_bsod_str("If problems continue, disable or remove any newly installed hardware\n");
    print_bsod_str("or software. Disable BIOS memory options such as caching or shadowing.\n\n");
    print_bsod_str("Technical Information:\n\n");
    print_bsod_str("*** STOP: 0x0000007B\n\n");
    print_bsod_str("*** kernel.org - Address 0xB00B1E55 base at Masix Core\n\n");

    print_bsod_str("Exception info: ");
    print_bsod_str(msg);
    print_bsod_str("\n");
}
