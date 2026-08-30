#include "kernel/vfs.h"
#include <string.h>

#define MAX_FD 32

vfs_node_t* fd_table[32];

extern int fs_create(const char* name);
extern int fs_read(int file_index, unsigned char* buffer, unsigned int size);
extern int fs_write(int file_index, const unsigned char* data, unsigned int size);
typedef struct { char name[32]; unsigned int size; int used; } FileEntry;
extern FileEntry file_table[];

static int32_t ramfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)offset;
    return fs_read(node->internal_id, buffer, size);
}

static int32_t ramfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, const uint8_t *buffer) {
    (void)offset;
    int32_t ret = fs_write(node->internal_id, buffer, size);
    node->size = file_table[node->internal_id].size;
    return ret;
}

static const char version_data[] = "MazukiOS 0.1.0 (Powered by musl libc, Sugomachip x86)\n";

static int32_t procfs_version_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node;
    if (offset >= sizeof(version_data)) return 0;
    if (offset + size > sizeof(version_data)) size = sizeof(version_data) - offset;
    for (uint32_t i = 0; i < size; i++) buffer[i] = version_data[offset + i];
    return size;
}

int32_t vfs_open(const char *path) {
    int fd = -1;
    for (int i = 3; i < MAX_FD; i++) {
        if (fd_table[i] == NULL) { fd = i; break; }
    }
    if (fd == -1) return -24; // -EMFILE

    extern void* malloc(size_t size);
    vfs_node_t* node = (vfs_node_t*)malloc(sizeof(vfs_node_t));
    if (!node) return -12; // -ENOMEM

    if (strcmp(path, "/proc/version") == 0 || strcmp(path, "proc/version") == 0) {
        strcpy(node->name, "version");
        node->flags = VFS_FILE;
        node->size = sizeof(version_data);
        node->internal_id = -1;
        node->read = procfs_version_read;
        node->write = NULL;
        fd_table[fd] = node;
        return fd;
    }

    int file_idx = -1;
    for (int i = 0; i < 64; i++) {
        if (file_table[i].used && strcmp(file_table[i].name, path) == 0) {
            file_idx = i;
            break;
        }
    }

    if (file_idx == -1) {
        file_idx = fs_create(path);
        if (file_idx == -1) { return -28; } // -ENOSPC
    }

    strcpy(node->name, file_table[file_idx].name);
    node->flags = VFS_FILE;
    node->size = file_table[file_idx].size;
    node->internal_id = file_idx;
    node->read = ramfs_read;
    node->write = ramfs_write;

    fd_table[fd] = node;
    return fd;
}
