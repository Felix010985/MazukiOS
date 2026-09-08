#include "kernel/vfs.h"
#include <string.h>
#include <version.h>
#include <procfs.h>

#define MAX_FD 32

// vfs_node_t* fd_table[32];
file_t fd_table[32];

extern int fs_create(const char* name);
extern int fs_read(int file_index, unsigned char* buffer, unsigned int size);
extern int fs_write(int file_index, const unsigned char* data, unsigned int size);
typedef struct { char name[32]; unsigned int size; int used; } FileEntry;
extern FileEntry file_table[];

static mountpoint_t *mount_list = NULL;

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

void vfs_mount_core(const char *path, fsdriver_t *driver) {
    /* Монтаж корня (/) и аллокация памяти к нему */
    extern void* malloc(size_t size);
    mountpoint_t *mp = (mountpoint_t*)malloc(sizeof(mountpoint_t));
    if (!mp) return;

    strncpy(mp->path, path, 32);
    mp->driver = driver;
    mp->next = mount_list;
    mount_list = mp;
}

// void vfs_init(void) {
//     /* Инициализация VFS и монтирование procfs */
//     extern fsdriver_t procfs_driver;
//     vfs_mount_core("/proc", &procfs_driver);
// }
void vfs_init(void) {
    for (int i = 0; i < 32; i++) {
        fd_table[i].type = FT_EMPTY;
        fd_table[i].offset = 0;
        fd_table[i].private_data = NULL;
    }

    fd_table[0].type = FT_DEVICE; // stdin
    fd_table[1].type = FT_DEVICE; // stdout
    fd_table[2].type = FT_DEVICE; // stderr

    // монтирование procfs
    extern fsdriver_t procfs_driver;
    vfs_mount_core("/proc", &procfs_driver);
}

int32_t vfs_open(const char *path) {
    int fd = -1;
    for (int i = 3; i < MAX_FD; i++) {
        if (fd_table[i].type == FT_EMPTY) { fd = i; break; }
    }
    if (fd == -1) return -24; // -EMFILE

    extern void* malloc(size_t size);
    vfs_node_t* node = (vfs_node_t*)malloc(sizeof(vfs_node_t));
    if (!node) return -12; // -ENOMEM
    /* Склеиваем путь к файлу с точки монтирования...
     * Освобождаем используемую память после
     * strncmp гарантирует (около 52%) что все пройдет успешно
     */
    for (mountpoint_t *mp = mount_list; mp != NULL; mp = mp->next) {
        size_t len = strlen(mp->path);
        if (strncmp(path, mp->path, len) == 0) {
            if (path[len] == '/' || path[len] == '\0') {
                const char *subpath = path + len;
                if (*subpath == '/') subpath++;

                int32_t res = mp->driver->open(subpath, node);
                if (res == 0) {
                    fd_table[fd].type = FT_VFS_FILE;
                    fd_table[fd].offset = 0;
                    fd_table[fd].private_data = (void*)node;

                    return fd;
                }

                extern void free(void* ptr);
                free(node);
                return res;
            }
        }
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
        if (file_idx == -1) {
            extern void free(void* ptr);
            free(node);
            return -28;
        } // -ENOSPC
    }

    strcpy(node->name, file_table[file_idx].name);
    node->flags = VFS_FILE;
    node->size = file_table[file_idx].size;
    node->internal_id = file_idx;
    node->read = ramfs_read;
    node->write = ramfs_write;

    fd_table[fd].type = FT_VFS_FILE;
    fd_table[fd].offset = 0;
    fd_table[fd].private_data = (void*)node;

    return fd;
}
