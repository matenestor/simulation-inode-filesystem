#ifndef FS_OPERATIONS_H
#define FS_OPERATIONS_H


#include <stdint.h>

#include "inc/_fs_varname.h"

#define SEPARATOR "/"

#define FS_READ(buffer, size, count)  fread(buffer, size, count, FS_VARIABLE_NAME)
#define FS_WRITE(buffer, size, count) fwrite(buffer, size, count, FS_VARIABLE_NAME)
#define FS_TELL                       ftell(FS_VARIABLE_NAME)
#define FS_SEEK_SET(offset)           fseek(FS_VARIABLE_NAME, offset, SEEK_SET)
#define FS_SEEK_CUR(offset)           fseek(FS_VARIABLE_NAME, offset, SEEK_CUR)

bool is_free_inode();
int32_t get_inodeid_by_path(char* path);


#endif
