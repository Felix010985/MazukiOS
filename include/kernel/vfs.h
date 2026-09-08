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

typedef struct vfs_node vfs_node_t;

typedef struct {
    int32_t (*open)(const char *subpath, vfs_node_t *node);
}   fsdriver_t;

typedef struct mountpoint {
    char path[32];               // Точка монтирования
    fsdriver_t *driver;          // Какой драйвер за неё отвечает
    struct mountpoint *next;     // Ссылочка на следующую точку (для списка)
} mountpoint_t;

typedef enum {
    FT_EMPTY = 0,
    FT_VFS_FILE, // Обычный файл
    FT_DEVICE    // Символьное устройство
} file_type_t;

typedef struct file {
    file_type_t type;    // Тип файла
    uint32_t offset;     // Оффсет чтения/записи
    void* private_data;  // Указатель на vfs_node_t ИЛИ на структуру устройства
} file_t;

extern file_t fd_table[32];

#endif
