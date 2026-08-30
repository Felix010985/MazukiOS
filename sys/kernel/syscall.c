#include <stdint.h>
#include <stddef.h>
#include <kernel/vfs.h>
#include "kernel/task.h"

extern int32_t k_sys_write(int fd, const char* buf, uint32_t count);
extern int32_t k_sys_read(int fd, char* buf, uint32_t count);
extern void idt_register_handler(uint8_t vector, uint32_t handler_addr, uint8_t flags);
extern void itoa(int value, char* str, int base);
extern void puts_com1(const char* s);

// struct iovec {
//     void* iov_base;
//     uint32_t iov_len;
// };

#define LINUX_SYS_EXIT            1
#define LINUX_SYS_FORK            2
#define LINUX_SYS_READ            3
#define LINUX_SYS_WRITE           4
#define LINUX_SYS_OPEN            5
#define LINUX_SYS_CLOSE           6
#define LINUX_SYS_EXECVE          11
#define LINUX_SYS_GETPID          20
#define LINUX_SYS_BRK             45
#define LINUX_SYS_IOCTL           54
#define LINUX_SYS_FCNTL           55
#define LINUX_SYS_GETDENTS        78

#define LINUX_SYS_FSTAT           108
#define LINUX_SYS_MODIFY_LDT      123
#define LINUX_SYS_WRITEV          146
#define LINUX_SYS_RT_SIGACTION    174

#define LINUX_SYS_RT_SIGPROCMASK  175
#define LINUX_SYS_GETCWD          183
#define LINUX_SYS_UGETRLIMIT      191
#define LINUX_SYS_MMAP2           192
#define LINUX_SYS_FCNTL64         221
#define LINUX_SYS_TKILL           238
#define LINUX_SYS_SET_THREAD_AREA 243

#define LINUX_SYS_EXIT_GROUP      252
#define LINUX_SYS_SET_TID_ADDRESS 258

#define LINUX_EBADF               9
#define LINUX_ENOSYS              38
#define LINUX_SYS_GETGID          64
#define LINUX_SYS_FSTAT64_ALT     147 // fstat64
#define LINUX_SYS_STAT64          195
#define LINUX_SYS_FSTAT64         197
#define LINUX_SYS_GETUID          199
#define LINUX_SYS_GETGID32        200
#define LINUX_SYS_GETEGID         201
#define LINUX_SYS_GETEUID         202
#define LINUX_SYS_GETPGID         132
#define LINUX_SYS_PSELECT6        308
#define LINUX_SYS_PRLIMIT64       340
#define LINUX_SYS_STATX           383
#define LINUX_SYS_CLOCK_GETTIME64 403




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

        case LINUX_SYS_EXIT:
            while(1);
            return 0;

        case LINUX_SYS_READ:
            return k_sys_read(regs->ebx, (char*)regs->ecx, regs->edx);

        case LINUX_SYS_WRITE:
            return k_sys_write(regs->ebx, (const char*)regs->ecx, regs->edx);

        case LINUX_SYS_BRK:
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

        case LINUX_SYS_FORK:
        {
            extern int32_t task_fork(struct syscall_regs* regs);
            return task_fork(regs);
        }

        case LINUX_SYS_MODIFY_LDT:
            return 0;

        case LINUX_SYS_SET_THREAD_AREA:
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

        case LINUX_SYS_WRITEV:
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

        case LINUX_SYS_SET_TID_ADDRESS:
            return 1;

        case LINUX_SYS_OPEN:
        {
            const char* path = (const char*)regs->ebx;
            if (path == NULL) return -14; // -EFAULT

            int32_t fd = vfs_open(path);
            if (fd < 0) {
                return -2;
            }
            return fd;
        }

        case LINUX_SYS_IOCTL:
            return 0;

        case LINUX_SYS_GETPID:
            extern task_t* current_task;
            return current_task->pid;

        case LINUX_SYS_RT_SIGPROCMASK:
            return 0;

        case LINUX_SYS_EXIT_GROUP:
        {
            puts_com1("SYS: Process called exit_group. Cleaning up...\n");

            extern void task_destroy(void);
            task_destroy();

            return 0;
        }

        case LINUX_SYS_EXECVE:
        {
            // ebx = const char* filename
            extern int32_t task_execve(const char* path, struct syscall_regs* regs);
            return task_execve((const char*)regs->ebx, regs);
        }

        case LINUX_SYS_FCNTL:
            return 0;

        case LINUX_SYS_FSTAT:
            return 0;

        case LINUX_SYS_RT_SIGACTION:
            return 0;

        case LINUX_SYS_TKILL:
            return 0;

        case LINUX_SYS_MMAP2:
        {
            uint32_t length = regs->ecx;

            uint32_t aligned_len = (length + 4095) & ~4095;

            uint32_t allocated_addr = current_process_brk;
            current_process_brk += aligned_len;

            return allocated_addr;
        }

        case LINUX_SYS_CLOSE:
            return 0;

        case LINUX_SYS_GETCWD:
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

        case LINUX_SYS_FCNTL64:
            // ebx = fd, ecx = cmd, edx = arg
            return 0;

        case LINUX_SYS_UGETRLIMIT:
            // ebx = resource, ecx = struct rlimit*
            return 0;

        case LINUX_SYS_GETDENTS:
            // ebx = fd, ecx = struct linux_dirent*, edx = count
            return 0;

        case LINUX_SYS_GETUID:
        case LINUX_SYS_GETEUID:
        case LINUX_SYS_GETGID:
        case LINUX_SYS_GETGID32:
        case LINUX_SYS_GETEGID:

            return 0;

        case LINUX_SYS_STAT64:
        case LINUX_SYS_FSTAT64:
        case LINUX_SYS_FSTAT64_ALT:
            // ebx = fd или путь, ecx = struct stat*
            return 0;

        case LINUX_SYS_CLOCK_GETTIME64:
            // ebx = clock_id, ecx = struct timespec64*
            return 0;

        case LINUX_SYS_GETPGID:
            // ebx = pid. Если ebx == 0, возвращаем PGID текущего процесса.
            return 1;

        case LINUX_SYS_PRLIMIT64:
            // ebx = pid, ecx = resource, edx = new_limit, esi = old_limit
            return 0;

        case LINUX_SYS_STATX:
            // ebx = dfd, ecx = filename, edx = flags, esi = mask, edi = buffer
            return 0;

        case LINUX_SYS_PSELECT6:
            // ebx = n, ecx = inp, edx = outp, esi = exp, edi = tsp
            return 0;

        default:
            char stub_buf[16];
            itoa(regs->eax, stub_buf, 10);

            puts_com1("SYS: Unimplemented Linux syscall requested: ");
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
