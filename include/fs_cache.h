#ifndef FS_CACHE_H
#define FS_CACHE_H

#include <stdio.h>

// cache size for fwriting and freading data blocks of filesystem
#define CACHE_SIZE 131072

extern FILE* filesystem;
extern struct superblock sb;
extern struct inode in_actual;

extern char fs_name[];
extern char buff_pwd[];
extern char buff_prompt[];

#endif
