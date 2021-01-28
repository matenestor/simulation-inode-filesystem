#ifndef FS_API_H
#define FS_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "inode.h"

// FILESYSTEM COMMON FUNCTIONS

void init_filesystem(const char*, bool*);
void close_filesystem();

// FILESYSTEM BITMAP FUNCTIONS

void bitmap_field_inodes_on(int32_t);
void bitmap_field_inodes_off(int32_t);
void bitmap_field_data_on(int32_t);
void bitmap_field_data_off(int32_t);
int32_t get_bitmap_field_inode();
int32_t get_bitmap_field_data();

// FILESYSTEM DATA BLOCK FUNCTIONS

// FILESYSTEM INODE FUNCTIONS

int32_t create_inode(struct inode*, enum item, int32_t);
int destroy_inode(struct inode*);
int32_t get_link(struct inode*);
int32_t get_inode_by_path(struct inode*, const char*);
int32_t get_path_to_root(char*, uint16_t, bool*);

// FILESYSTEM INPUT/OUTPUT FUNCTIONS

void fs_seek_bm_inodes(uint32_t);
void fs_seek_bm_data(uint32_t);
void fs_seek_inodes(uint32_t);
void fs_seek_data(uint32_t);
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
