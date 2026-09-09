/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Masix Kernel
 * Copyright (C) 2026, FelixProfi. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */
#include <vfs.h>
#include <string.h>
#include <version.h>

int32_t procfs_version_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    if (offset >= sizeof(version_data)) return 0;
    if (offset + size > sizeof(version_data)) size = sizeof(version_data) - offset;
    for (uint32_t i = 0; i < size; i++) buffer[i] = version_data[offset + i];
    return size;
}

int32_t procfs_meminfo_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    char mem_data[128];
    char num_buf[32];
    // Тестовые значения памяти для /proc/meminfo
    uint32_t total_kb = 16384232;
    uint32_t free_kb  = 12543104;

    strcpy(mem_data, "MemTotal:       ");
    itoa(total_kb, num_buf, 10);
    strcpy(mem_data + strlen(mem_data), num_buf);
    strcpy(mem_data + strlen(mem_data), " kB\nMemFree:        ");
    itoa(free_kb, num_buf, 10);
    strcpy(mem_data + strlen(mem_data), num_buf);
    strcpy(mem_data + strlen(mem_data), " kB\n");

    uint32_t len = strlen(mem_data);
    if (offset >= len) return 0;
    if (offset + size > len) size = len - offset;

    for (uint32_t i = 0; i < size; i++) buffer[i] = mem_data[offset + i];
    return size;
}

int32_t procfs_open_file(const char *subpath, vfs_node_t *node) {
    if (strcmp(subpath, "version") == 0) {
        strcpy(node->name, "version");
        node->flags = VFS_FILE;
        node->size = sizeof(version_data);
        node->internal_id = -1;
        node->read = procfs_version_read;
        node->write = NULL;
        return 0;
    }
    if (strcmp(subpath, "meminfo") == 0) {
        strcpy(node->name, "meminfo");
        node->flags = VFS_FILE;
        node->size = 128;
        node->internal_id = -2;
        node->read = procfs_meminfo_read;
        node->write = NULL;
        return 0;
    }
    return -2; // -ENOENT
}

fsdriver_t procfs_driver = {
    .open = procfs_open_file
};
