#ifndef TASK_H
#define VFS_H

#include <stdint.h>

#define TASK_RUNNING 0
#define TASK_READY   1

typedef struct task {
    uint32_t pid;
    uint32_t esp;
    uint32_t state;
    uint8_t* kernel_stack;
    uint8_t* user_stack;
    struct task* next;
} task_t;

void task_init(void);
void task_create(void* entry_point);
void schedule(void);
void task_destroy(void);

extern task_t* current_task;

#endif
