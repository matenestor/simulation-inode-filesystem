#ifndef FS_OPERATIONS_H
#define FS_OPERATIONS_H


#include <stdint.h>

#include "inc/inode.h"


// I think I did something very wrong
// with purpose of this enum probably,
enum search_by {
    Id_by_name,
    Name_by_id
};

void fs_seek_set(unsigned int);
long int fs_tell();
void fs_flush();
unsigned int fs_read_superblock(struct superblock*, size_t, size_t);
unsigned int fs_read_inode(struct inode*, size_t, size_t);
unsigned int fs_read_directory_item(struct directory_item*, size_t, size_t);
unsigned int fs_read_int32t(int32_t*, size_t, size_t);
unsigned int fs_read_bool(bool*, size_t, size_t);
unsigned int fs_read_char(char*, size_t, size_t);
unsigned int fs_write_superblock(const struct superblock*, size_t, size_t);
unsigned int fs_write_inode(const struct inode*, size_t, size_t);
unsigned int fs_write_directory_item(const struct directory_item*, size_t, size_t);
unsigned int fs_write_int32t(const int32_t*, size_t, size_t);
unsigned int fs_write_bool(const bool*, size_t, size_t);
unsigned int fs_write_char(const char*, size_t, size_t);

int32_t get_inode_by_path(struct inode*, const char*);
int32_t get_path_to_root(char*, uint16_t, bool*);
int32_t create_inode(struct inode*, enum item, int32_t);

int32_t get_link(struct inode*);

#endif
