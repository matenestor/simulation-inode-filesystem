#ifndef FS_OP_H
#define FS_OP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "inode.h"


void fs_seek_bm_inodes(const uint32_t);
void fs_seek_bm_blocks(const uint32_t);
void fs_seek_inodes(const uint32_t);
void fs_seek_blocks(const uint32_t);
void fs_flush();
unsigned int fs_read_superblock(struct superblock*, size_t);
unsigned int fs_read_inode(struct inode*, size_t);
unsigned int fs_read_directory_item(struct directory_item*, size_t);
unsigned int fs_read_int32t(int32_t*, size_t);
unsigned int fs_read_bool(bool*, size_t);
unsigned int fs_read_char(char*, size_t);
unsigned int fs_write_superblock(const struct superblock*, size_t);
unsigned int fs_write_inode(const struct inode*, size_t);
unsigned int fs_write_directory_item(const struct directory_item*, size_t);
unsigned int fs_write_int32t(const int32_t*, size_t);
unsigned int fs_write_bool(const bool*, size_t);
unsigned int fs_write_char(const char*, size_t);

#endif
