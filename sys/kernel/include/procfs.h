#ifndef PROCFS_H
#define PROCFS_H
#include <vfs.h>
int32_t procfs_meminfo_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
int32_t procfs_version_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
#endif
