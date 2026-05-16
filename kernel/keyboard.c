#include "kernel/keyboard.h"
#include "kernel/io.h"
#include <stdint.h>

#define KBD_DATA   0x60
#define KBD_STATUS 0x64

#define KBD_BUFFER_SIZE 256
static char kbd_buffer[KBD_BUFFER_SIZE];
static int kbd_head = 0;
static int kbd_tail = 0;

static int shift = 0;

static const char scancode_table[128] = {
    0, 27,'1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,' ',0,
};

static const char scancode_shift_table[128] = {
    0, 27,'!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,
    '*',0,' ',0,
};

static void kbd_push(char c) {
    int next = (kbd_head + 1) % KBD_BUFFER_SIZE;
    if (next != kbd_tail) {
        kbd_buffer[kbd_head] = c;
        kbd_head = next;
    }
}

void keyboard_handler_c(void) {
    if (inb(KBD_STATUS) & 1) {
        uint8_t sc = inb(KBD_DATA);

        if (sc & 0x80) {
            sc &= 0x7F;
            if (sc == 42 || sc == 54) shift = 0;
            return;
        }
        if (sc == 42 || sc == 54) {
            shift = 1;
            return;
        }

        char c = shift ? scancode_shift_table[sc] : scancode_table[sc];
        if (c) {
            kbd_push(c);
        }
    }
}

char keyboard_getc(void) {
    while (1) {
        if (kbd_head != kbd_tail) {
            char c = kbd_buffer[kbd_tail];
            kbd_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
            return c;
        }
        //__asm__ volatile("hlt");
    }
}
