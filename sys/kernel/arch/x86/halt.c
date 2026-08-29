#include "kernel/halt.h"

void halt(void) {
    for (;;) {
        __asm__ __volatile__("hlt");
    }
}
