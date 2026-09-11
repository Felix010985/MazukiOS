/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Masix Kernel
 * Copyright (C) 2026, FelixProfi. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */
#include <stdint.h>
#include <stddef.h>
#include <vfs.h>
#include <task.h>

extern int32_t k_sys_write(int fd, const char* buf, uint32_t count);
extern int32_t k_sys_read(int fd, char* buf, uint32_t count);
extern void idt_register_handler(uint8_t vector, uint32_t handler_addr, uint8_t flags);
extern void itoa(int value, char* str, int base);
extern void puts_com1(const char* s);

// struct iovec {
//     void* iov_base;
//     uint32_t iov_len;
// };

#define LINUX_RESTART_SYSCALL
#define MASIX_EXIT            1
#define MASIX_FORK            2
#define MASIX_READ            3
#define MASIX_WRITE           4
#define MASIX_OPEN            5
#define MASIX_CLOSE           6
#define MASIX_EXECVE          11
#define MASIX_GETPID          20
#define MASIX_BRK             45
#define MASIX_IOCTL           54
#define MASIX_FCNTL           55
#define MASIX_GETDENTS        78
#define MASIX_MUNMAP          91

#define MASIX_FSTAT           108
#define MASIX_MODIFY_LDT      123
#define MASIX__LLSEEK         140
#define MASIX_WRITEV          146
#define MASIX_RT_SIGACTION    174

#define MASIX_RT_SIGPROCMASK  175
#define MASIX_GETCWD          183
#define MASIX_UGETRLIMIT      191
#define MASIX_MMAP2           192
#define MASIX_FCNTL64         221
#define MASIX_TKILL           238
#define MASIX_SET_THREAD_AREA 243

#define MASIX_EXIT_GROUP      252
#define MASIX_SET_TID_ADDRESS 258

#define LINUX_EBADF           9
#define LINUX_ENOSYS          38
#define MASIX_GETGID          64
#define MASIX_FSTAT64_ALT     147 // fstat64
#define MASIX_STAT64          195
#define MASIX_FSTAT64         197
#define MASIX_GETUID          199
#define MASIX_GETGID32        200
#define MASIX_GETEGID         201
#define MASIX_GETEUID         202
#define MASIX_GETPGID         132
#define MASIX_PSELECT6        308
#define MASIX_PRLIMIT64       340
#define MASIX_STATX           383
#define MASIX_CLOCK_GETTIME64 403




struct syscall_regs {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
};

uint32_t current_process_brk = 0x01100000;

uint32_t syscall_handler_c(struct syscall_regs* regs) {
    switch (regs->eax) {

        case MASIX_EXIT:
            while(1);
            return 0;

        case MASIX_READ:
        {
            int fd = regs->ebx;
            char* user_buf = (char*)regs->ecx;
            uint32_t count = regs->edx;

            if (fd < 3) {
                extern int32_t k_sys_read(int fd, char* buf, uint32_t count);
                return k_sys_read(fd, user_buf, count);
            }

            struct fd_entry {
                int type;
                uint32_t offset;
                void* private_data;
            };

            // extern struct fd_entry fd_table[32];

            if (fd >= 32 || fd_table[fd].type == 0) { // FT_EMPTY
                return -9; // -EBADF
            }

            vfs_node_t* node = (vfs_node_t*)fd_table[fd].private_data;

            if (node && node->read != NULL) {
                int32_t res = node->read(node, fd_table[fd].offset, count, (uint8_t*)user_buf);
                if (res > 0) {
                    fd_table[fd].offset += res;
                }
                return res;
            }

            return 0;
        }

        case MASIX_WRITE:
            return k_sys_write(regs->ebx, (const char*)regs->ecx, regs->edx);

        case MASIX_BRK:
        {
            uint32_t new_brk = regs->ebx;

            if (new_brk == 0) {
                return current_process_brk;
            }

            if (new_brk >= current_process_brk) {
                current_process_brk = new_brk;
            }
            return current_process_brk;
        }

        case MASIX_FORK:
        {
            extern int32_t task_fork(struct syscall_regs* regs);
            return task_fork(regs);
        }

        case MASIX_MODIFY_LDT:
            return 0;

        case MASIX_SET_THREAD_AREA:
        {
            uint32_t* user_desc = (uint32_t*)regs->ebx;

            if (user_desc == NULL) return -9; // -EFAULT

            if ((int32_t)user_desc[0] == -1) {
                user_desc[0] = 6;
            }

            uint32_t entry_number = user_desc[0];
            uint32_t base_addr    = user_desc[1];

            extern void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
            gdt_set_entry(entry_number, base_addr, 0xFFFFF, 0xF2, 0xCF);

            return 0;
        }

        case MASIX_WRITEV:
        {
            // ebx = fd (1 для stdout, 2 для stderr)
            // ecx = указатель на массив структур struct iovec
            // edx = количество элементов в этом массиве (iovcnt)
            int fd = regs->ebx;
            const struct iovec* iov = (const struct iovec*)regs->ecx;
            int iovcnt = regs->edx;

            if (iov == NULL) return -9; // -EFAULT

            int32_t total_written = 0;

            for (int i = 0; i < iovcnt; i++) {
                if (iov[i].iov_base != NULL && iov[i].iov_len > 0) {

                    extern int32_t k_sys_write(int fd, const char* buf, uint32_t count);
                    int32_t ret = k_sys_write(fd, (const char*)iov[i].iov_base, iov[i].iov_len);

                    if (ret < 0) return ret;
                    total_written += ret;
                }
            }

            return total_written;
        }

        case MASIX_SET_TID_ADDRESS:
            return 1;

        case MASIX_OPEN:
        {
            const char* path = (const char*)regs->ebx;
            if (path == NULL) return -14; // -EFAULT

            int32_t fd = vfs_open(path);
            if (fd < 0) {
                return -2;
            }
            return fd;
        }

        case MASIX_IOCTL:
            return 0;

        case MASIX_GETPID:
            extern task_t* current_task;
            return current_task->pid;

        case MASIX_RT_SIGPROCMASK:
            return 0;

        case MASIX_EXIT_GROUP:
        {
            puts_com1("SYS: Process called exit_group. Cleaning up...\n");

            extern void task_destroy(void);
            task_destroy();

            return 0;
        }

        case MASIX_EXECVE:
        {
            // ebx = const char* filename
            extern int32_t task_execve(const char* path, struct syscall_regs* regs);
            return task_execve((const char*)regs->ebx, regs);
        }

        case MASIX_FCNTL:
            return 0;

        case MASIX_FSTAT:
            return 0;

        case MASIX_RT_SIGACTION:
            return 0;

        case MASIX_TKILL:
            return 0;

        case MASIX_MMAP2:
        {
            uint32_t length = regs->ecx;

            uint32_t aligned_len = (length + 4095) & ~4095;

            uint32_t allocated_addr = current_process_brk;
            current_process_brk += aligned_len;

            return allocated_addr;
        }

        case MASIX_CLOSE:
            return 0;

        case MASIX_GETCWD:
        {
            // ebx = char* buf, ecx = unsigned long size
            char* user_buf = (char*)regs->ebx;
            uint32_t size = regs->ecx;

            if (user_buf != NULL && size > 2) {
                user_buf[0] = '/';
                user_buf[1] = '\0';
                return (uint32_t)user_buf;
            }
            return 0;
        }

        case MASIX_FCNTL64:
            // ebx = fd, ecx = cmd, edx = arg
            return 0;

        case MASIX_UGETRLIMIT:
            // ebx = resource, ecx = struct rlimit*
            return 0;

        case MASIX_GETDENTS:
            // ebx = fd, ecx = struct linux_dirent*, edx = count
            return 0;

        case MASIX_GETUID:
        case MASIX_GETEUID:
        case MASIX_GETGID:
        case MASIX_GETGID32:
        case MASIX_GETEGID:

            return 0;

        case MASIX_STAT64:
        case MASIX_FSTAT64:
        case MASIX_FSTAT64_ALT:
            // ebx = fd или путь, ecx = struct stat*
            return 0;

        case MASIX_CLOCK_GETTIME64:
            // ebx = clock_id, ecx = struct timespec64*
            return 0;

        case MASIX_GETPGID:
            // ebx = pid. Если ebx == 0, возвращаем PGID текущего процесса.
            return 1;

        case MASIX_PRLIMIT64:
            // ebx = pid, ecx = resource, edx = new_limit, esi = old_limit
            return 0;

        case MASIX_STATX:
            // ebx = dfd, ecx = filename, edx = flags, esi = mask, edi = buffer
            return 0;

        case MASIX_PSELECT6:
            // ebx = n, ecx = inp, edx = outp, esi = exp, edi = tsp
            return 0;

        case MASIX_MUNMAP:
        {
            // ebx = addr, ecx = length
            return 0;
        }

        case MASIX__LLSEEK:
        {
            // ebx = fd
            // ecx = offset_high
            // edx = offset_low
            // esi = uint64_t* result_ptr
            // edi = whence (SEEK_SET, SEEK_CUR, SEEK_END)
            int fd = regs->ebx;
            uint32_t offset_high = regs->ecx;
            uint32_t offset_low = regs->edx;
            uint64_t* res_ptr = (uint64_t*)regs->esi;
            uint32_t whence = regs->edi;

            struct fd_entry {
                int type;
                uint32_t offset;
                void* private_data;
            };
            //extern struct fd_entry fd_table[32];

            if (fd >= 32 || fd_table[fd].type == 0) {
                return -9; // -EBADF
            }

            vfs_node_t* node = (vfs_node_t*)fd_table[fd].private_data;
            uint64_t offset = ((uint64_t)offset_high << 32) | offset_low;

            if (whence == 0) {         // SEEK_SET
                fd_table[fd].offset = (uint32_t)offset;
            } else if (whence == 1) {  // SEEK_CUR
                fd_table[fd].offset += (uint32_t)offset;
            } else if (whence == 2) {  // SEEK_END
                fd_table[fd].offset = node->size + (uint32_t)offset;
            } else {
                return -22; // -EINVAL
            }

            if (res_ptr != NULL) {
                *res_ptr = (uint64_t)fd_table[fd].offset;
            }

            return 0;
        }

        default:
            char stub_buf[16];
            itoa(regs->eax, stub_buf, 10);

            puts_com1("Masix: SYS: Unimplemented Linux syscall requested: ");
            puts_com1(stub_buf);
            puts_com1("\n");

            return -LINUX_ENOSYS;
    }
}

__attribute__((naked)) void syscall_handler_asm(void) {
    __asm__ __volatile__ (
        "pusha \n\t"

        "mov $0x10, %ax \n\t"
        "mov %ax, %ds \n\t"
        "mov %ax, %es \n\t"

        "sti \n\t"

        "push %esp \n\t"
        "call syscall_handler_c \n\t"
        "add $4, %esp \n\t"

        "mov %eax, 28(%esp) \n\t"

        "cli \n\t"

        "popa \n\t"

        "push %ax \n\t"
        "mov $0x23, %ax \n\t"
        "mov %ax, %ds \n\t"
        "mov %ax, %es \n\t"
        "pop %ax \n\t"

        "iret \n\t"
    );
}

void syscall_init(void) {
    idt_register_handler(0x80, (uint32_t)syscall_handler_asm, 0xEE);
}
