#ifndef FS_CACHE_H
#define FS_CACHE_H


#include <stdio.h>

#include "_fs_varname.h"

/** Cache size for fwriting and freading data blocks of filesystem */
#define CACHE_SIZE 8192

extern FILE* FS_VARIABLE_NAME;
extern struct superblock sb;
extern struct inode in_actual;


#endif
