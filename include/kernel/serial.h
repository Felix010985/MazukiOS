#define COM1 0x3F8

// static inline void outb(uint16_t port, uint8_t val) {}

// static inline uint8_t inb(uint16_t port) {}

void init_serial(void);

void putc_com1(char a);

void puts_com1(const char* str);
