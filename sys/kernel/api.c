#include "kernel/api.h"
#include "kernel/vga.h"
#include "kernel/keyboard.h"
#include "kernel/alloc.h"
#include "kernel/vfs.h"
#include <stdint.h>

extern void tty_write_char(char c);
extern char keyboard_getc(void);
extern vfs_node_t* fd_table[];

int32_t k_sys_write(int fd, const char* buf, uint32_t count) {
    if (fd == 1 || fd == 2) {
        for (uint32_t i = 0; i < count; i++) tty_write_char(buf[i]);
        return count;
    }

    if (fd >= 3 && fd < 32 && fd_table[fd] != NULL) {
        if (fd_table[fd]->write) {
            return fd_table[fd]->write(fd_table[fd], 0, count, (const uint8_t*)buf);
        }
        return -22; // -EINVAL
    }
    return -9; // -EBADF
}

int32_t k_sys_read(int fd, char* buf, uint32_t count) {
    if (fd == 0) {
        if (count == 0) return 0;

        uint32_t read_bytes = 0;

        while (read_bytes < count) {
            char c = keyboard_getc();

            if (c == '\n' || c == '\r') {
                buf[read_bytes++] = '\n';
                tty_write_char('\n');
                break;
            }
            else if (c == '\b') {
                if (read_bytes > 0) {
                    read_bytes--;
                    tty_write_char('\b');
                }
            }
            else {
                buf[read_bytes++] = c;
                tty_write_char(c);

                if (read_bytes == count) {
                    break;
                }
            }
        }

        return read_bytes;
    }

    if (fd >= 3 && fd < 32 && fd_table[fd] != NULL) {
        if (fd_table[fd]->read) {
            return fd_table[fd]->read(fd_table[fd], 0, count, (uint8_t*)buf);
        }
    }
    return -9; // -EBADF
}

void sys_putc(char c) {
    vga_putc(c);
}

void sys_cls(void) {
    k_sys_write(1, "\033[2J\033[H", 7);
}

void* malloc(size_t size) {
    return alloc(size);
}

void free(void* ptr) {
    alloc_free(ptr);
}
