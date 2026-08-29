#include <stdint.h>

#define COM1 0x3F8

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void init_serial(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
}

int is_transmit_empty(void) {
    return inb(COM1 + 5) & 0x20;
}

void putc_com1(char a) {
    while (is_transmit_empty() == 0);
    outb(COM1, a);
}

void puts_com1(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        putc_com1(str[i]);
    }
}
// не готово и будет еще поправлено в будущем, пока вариант от меня и ИИшки от гугла, посмотрим как оно себя поведет
