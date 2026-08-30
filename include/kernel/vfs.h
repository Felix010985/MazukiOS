#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

#define VFS_FILE      1
#define VFS_DIRECTORY 2

struct vfs_node;

typedef int32_t (*vfs_read_t)(struct vfs_node *node, uint32_t offset, uint32_t size, uint8_t *buffer);
typedef int32_t (*vfs_write_t)(struct vfs_node *node, uint32_t offset, uint32_t size, const uint8_t *buffer);

typedef struct iovec {
    void* iov_base;
    uint32_t iov_len;
} iovec_t;

typedef struct vfs_node {
    char name[32];
    uint32_t flags;
    uint32_t size;
    int32_t  internal_id;

    vfs_read_t  read;
    vfs_write_t write;
} vfs_node_t;

void vfs_init(void);

int32_t vfs_open(const char *path);
int32_t vfs_read(int fd, void *buf, uint32_t count);
int32_t vfs_write(int fd, const void *buf, uint32_t count);

#endif
