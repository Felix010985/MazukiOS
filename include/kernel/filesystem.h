#ifndef FS_H
#define FS_H

#include <stdint.h>

#define DISK_SIZE 1024*1024
#define BLOCK_SIZE 512
#define MAX_FILES 64
#define MAX_BLOCKS_PER_FILE 10
#define TOTAL_BLOCKS (DISK_SIZE / BLOCK_SIZE)

extern unsigned char disk[DISK_SIZE];
extern unsigned char block_bitmap[TOTAL_BLOCKS/8];

int is_block_free(int index);
void set_block_used(int index);
void set_block_free(int index);
int allocate_block(void);

typedef struct {
    char name[32];
    unsigned int size;
    int blocks[MAX_BLOCKS_PER_FILE];
    int used;
} FileEntry;

extern FileEntry file_table[MAX_FILES];

// операции с файлами
int fs_create(const char* name);
void fs_delete(int index);
int fs_write(int file_index, const unsigned char* data, unsigned int size);
int fs_read(int file_index, unsigned char* buffer, unsigned int size);

#endif
// честно я спросил chatgpt зачем надо первые 2 строки и он сказал для безопасности кода, оставлю так пока :)
