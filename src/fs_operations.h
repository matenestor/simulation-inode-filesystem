#ifndef FS_OPERATIONS_H
#define FS_OPERATIONS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "inode.h"


int32_t get_inode_by_path(struct inode*, const char*);
int32_t get_path_to_root(char*, uint16_t, bool*);
int32_t create_inode(struct inode*, enum item, int32_t);
int destroy_inode(struct inode*);

int32_t get_link(struct inode*);

#endif
