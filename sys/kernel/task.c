#include "kernel/task.h"
#include "kernel/alloc.h"
#include <string.h>

task_t* current_task = NULL;
static task_t* task_list_head = NULL;
static uint32_t next_pid = 1;

extern void* malloc(size_t size);

void task_init(void) {
    task_t* init_task = (task_t*)malloc(sizeof(task_t));
    init_task->pid = next_pid++;
    init_task->state = TASK_RUNNING;
    init_task->esp = 0;
    init_task->kernel_stack = (uint8_t*)malloc(4096);
    init_task->user_stack = (uint8_t*)malloc(4096);

    init_task->next = init_task;
    task_list_head = init_task;
    current_task = init_task;
}

void task_create(void* entry_point) {
    task_t* new_task = (task_t*)malloc(sizeof(task_t));
    new_task->pid = next_pid++;
    new_task->state = TASK_READY;
    new_task->kernel_stack = (uint8_t*)malloc(4096);
    new_task->user_stack = (uint8_t*)malloc(4096);

    uint32_t* ustack = (uint32_t*)((uint32_t)new_task->user_stack + 4096);

    ustack[-1] = 0;  // envp[0] = NULL
    ustack[-2] = 0;  // argv[1] = NULL
    ustack[-3] = (uint32_t)entry_point; // argv[0]
    ustack[-4] = 1;  // argc = 1

    uint32_t* kstack = (uint32_t*)((uint32_t)new_task->kernel_stack + 4096);

    kstack[-1] = 0x23;                              // User SS
    kstack[-2] = (uint32_t)new_task->user_stack + 4096 - 16; // User ESP
    kstack[-3] = 0x0202;                            // EFLAGS
    kstack[-4] = 0x1B;                              // User CS
    kstack[-5] = (uint32_t)entry_point;             // User EIP

    kstack[-6] = 0;  // EAX
    kstack[-7] = 0;  // ECX
    kstack[-8] = 0;  // EDX
    kstack[-9] = 0;  // EBX
    kstack[-10] = 0; // ESP
    kstack[-11] = 0; // EBP
    kstack[-12] = 0; // ESI
    kstack[-13] = 0; // EDI

    new_task->esp = (uint32_t)kstack - (13 * 4);

    new_task->next = task_list_head->next;
    task_list_head->next = new_task;
}

void schedule(void) {
    if (!current_task) return;
    current_task = current_task->next;
}

struct syscall_regs {
    uint32_t edi; uint32_t esi; uint32_t ebp; uint32_t esp;
    uint32_t ebx; uint32_t edx; uint32_t ecx; uint32_t eax;
};

int32_t task_fork(struct syscall_regs* regs) {
    task_t* child = (task_t*)malloc(sizeof(task_t));
    if (!child) return -12; // -ENOMEM

    child->pid = next_pid++;
    child->state = TASK_READY;
    child->kernel_stack = (uint8_t*)malloc(4096);
    child->user_stack = (uint8_t*)malloc(4096);

    memcpy(child->user_stack, current_task->user_stack, 4096);

    memcpy(child->kernel_stack, current_task->kernel_stack, 4096);

    int32_t kstack_offset = (int32_t)child->kernel_stack - (int32_t)current_task->kernel_stack;
    int32_t ustack_offset = (int32_t)child->user_stack - (int32_t)current_task->user_stack;

    child->esp = (uint32_t)regs + kstack_offset;

    struct syscall_regs* child_regs = (struct syscall_regs*)child->esp;

    uint32_t* child_iret_frame = (uint32_t*)((uint32_t)child->kernel_stack + 4096);

    child_iret_frame[-2] += ustack_offset;

    child_regs->ebp += ustack_offset;

    child_regs->eax = 0;

    child->next = task_list_head->next;
    task_list_head->next = child;

    return child->pid;
}

extern void free(void* ptr);
extern void puts_com1(char* a);

void task_destroy(void) {

    if (current_task->next == current_task) {
        puts_com1("Masix: Init process tried to exit! System halted.\n");
        for (;;) { asm volatile("hlt"); }
    }

    task_t* dead_task = current_task;

    task_t* prev = task_list_head;
    while (prev->next != dead_task) {
        prev = prev->next;
    }

    prev->next = dead_task->next;

    if (task_list_head == dead_task) {
        task_list_head = dead_task->next;
    }

    current_task = dead_task->next;

    free(dead_task->user_stack);
    free(dead_task->kernel_stack);
    free(dead_task);

    puts_com1("Masix: Dead task resources fully cleaned from kernel heap.\n");

    __asm__ __volatile__ (
        "movl current_task, %%eax \n\t"
        "movl 4(%%eax), %%esp \n\t"

    "movl 12(%%eax), %%ebx \n\t"
    "addl $4096, %%ebx \n\t"
    "pushl %%ebx \n\t"
    "pushl $0x10 \n\t"
    "pushl $5 \n\t"
    "call write_tss \n\t"
    "addl $12, %%esp \n\t"

    "popa \n\t"
    "iret \n\t"
    :
    :
    : "eax", "ebx", "memory"
    );
}
int32_t task_execve(const char* path, struct syscall_regs* regs) {
    (void)regs;
    extern uint32_t shell_elf_start;
    extern uint32_t shell_elf_size;

    if (shell_elf_start == 0 || shell_elf_size == 0) {
        return -2; // -ENOENT
    }

    puts_com1("Masix: sys_execve reloading process image...\n");

    extern void* elf_load_binary(uint32_t file_start);
    void* entry_point = elf_load_binary(shell_elf_start);

    if (entry_point == NULL) {
        return -8; // -ENOEXEC
    }

    uint32_t* ustack = (uint32_t*)((uint32_t)current_task->user_stack + 4096);
    ustack[-1] = 0; // envp = NULL
    ustack[-2] = 0; // argv = NULL
    ustack[-3] = 0; // argc = 0

    uint32_t* kstack_iret = (uint32_t*)((uint32_t)current_task->kernel_stack + 4096);
    kstack_iret[-2] = (uint32_t)ustack - 12;
    kstack_iret[-5] = (uint32_t)entry_point;

    regs->eax = 0;
    regs->ebx = 0;
    regs->ecx = 0;
    regs->edx = 0;
    regs->esi = 0;
    regs->edi = 0;
    regs->ebp = 0;

    puts_com1("Masix: sys_execve switch complete!\n");
    return 0;
}
