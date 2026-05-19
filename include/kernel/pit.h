#ifndef PIT_H
#define PIT_H

#include <stdint.h>

#define PIT_BASE_FREQUENCY 1193182

#define PIT_PORT_DATA0    0x40
#define PIT_PORT_COMMAND  0x43

void pit_init(uint32_t frequency);

uint32_t pit_get_ticks(void);

void sleep(uint32_t ticks);

#endif
