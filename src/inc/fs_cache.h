#ifndef FS_CACHE_H
#define FS_CACHE_H


#include <stdio.h>

/** Cache size for fwriting and freading data blocks of filesystem */
#define CACHE_SIZE 8192

extern FILE* filesystem;
extern struct superblock sb;
extern struct inode in_actual;
extern char buff_prompt[];

#endif
