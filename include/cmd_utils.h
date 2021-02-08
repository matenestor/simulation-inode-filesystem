#ifndef CMD_UTILS_H
#define CMD_UTILS_H

#include <stdbool.h>

#include "inode.h"


int split_path(const char* path, char* dir_path, char* ir_name);
bool item_exists(const struct inode* inode_parent, const char* dir_name);

#endif
