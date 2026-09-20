/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Masix Kernel
 * Copyright (C) 2026, FelixProfi. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */
#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define TASK_RUNNING 0
#define TASK_READY   1

#define MAX_PATH 256

typedef struct task {
    uint32_t pid;
    uint32_t esp;
    uint32_t cr3;
    uint32_t state;
    uint8_t* kernel_stack;
    uint8_t* user_stack;
    struct task* next;
    char cwd[MAX_PATH];
} task_t;

void task_init(void);
void task_create(void* entry_point);
void schedule(void);
void task_destroy(void);

extern task_t* current_task;

#endif
