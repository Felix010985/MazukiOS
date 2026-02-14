#include "kernel/keyboard.h"
#include <stdint.h>

#define KBD_DATA   0x60
#define KBD_STATUS 0x64

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

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

static void cpu_relax(void) {
    for (volatile int i=0; i<1000; i++);
}

char keyboard_getc(void) {
    while (1) {
        if (inb(KBD_STATUS) & 1) {
            uint8_t sc = inb(KBD_DATA);

            if (sc & 0x80) { sc &= 0x7F; if (sc==42||sc==54) shift=0; continue; }
            if (sc==42||sc==54) { shift=1; continue; }

            char c = shift ? scancode_shift_table[sc] : scancode_table[sc];
            if (c) return c;
        } else {
            cpu_relax();
        }
    }
}
