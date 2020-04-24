#ifndef FS_OPERATIONS_H
#define FS_OPERATIONS_H


#include <stdint.h>

#include "inc/_fs_varname.h"
#include "inc/inode.h"

#define SEPARATOR "/"

#define FS_READ(buffer, size, count)  fread(buffer, size, count, FS_VARIABLE_NAME)
#define FS_WRITE(buffer, size, count) fwrite(buffer, size, count, FS_VARIABLE_NAME)
#define FS_TELL                       ftell(FS_VARIABLE_NAME)
#define FS_SEEK_SET(offset)           fseek(FS_VARIABLE_NAME, offset, SEEK_SET)
#define FS_SEEK_CUR(offset)           fseek(FS_VARIABLE_NAME, offset, SEEK_CUR)
#define FS_FLUSH					  fflush(FS_VARIABLE_NAME)

int32_t get_inode_by_path(struct inode*, const char*);
int32_t create_inode(struct inode*, enum item, int32_t);

//int32_t get_last_link_value(const struct inode*);
int32_t get_link(struct inode*);

#endif
