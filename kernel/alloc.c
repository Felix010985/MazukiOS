#include <stddef.h>
#include <stdint.h>

#define HEAP_SIZE 0x10000
static uint8_t heap[HEAP_SIZE];
static size_t heap_index = 0;

void* alloc(size_t size) {
    if (heap_index + size > HEAP_SIZE) return 0;
    void* ptr = &heap[heap_index];
    heap_index += size;
    return ptr;
}

void alloc_free(void* ptr) {
    (void)ptr;
}
