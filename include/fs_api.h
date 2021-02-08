#ifndef FS_API_H
#define FS_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "inode.h"

// FILESYSTEM COMMON FUNCTIONS

int init_filesystem(const char* fsp, bool* is_formatted);
void close_filesystem();

// FILESYSTEM BITMAP FUNCTIONS

uint32_t allocate_bitmap_field_inode();
uint32_t allocate_bitmap_field_data();
void free_bitmap_field_inode(int32_t id);
void free_bitmap_field_data(int32_t id);

// FILESYSTEM DATA BLOCK FUNCTIONS

int init_block_with_directories(uint32_t id_block);
int init_empty_dir_block(struct directory_item* block, uint32_t id_self, uint32_t id_parent);

// FILESYSTEM LINK FUNCTIONS

int free_all_links(struct inode* in_source);
int create_empty_links(uint32_t* buffer, size_t to_create, struct inode* inode_source);

// FILESYSTEM INODE FUNCTIONS

int free_inode_file(struct inode* id_inode);
int free_inode_directory(struct inode* id_inode);
uint32_t create_inode_file(struct inode* new_inode);
uint32_t create_inode_directory(struct inode* new_inode, uint32_t id_parent);

// FILESYSTEM UTILS FUNCTIONS

int get_inode(struct inode* inode_dest, const char* path);
int get_inode_wparent(struct inode* inode_dest, struct inode* inode_parent, const char* path);
int get_path_to_root(char* dest_path, size_t length_path, const struct inode* inode_source);
bool is_directory_empty(const struct inode* inode_source);

// FILESYSTEM INPUT/OUTPUT FUNCTIONS

// read
size_t fs_read_inode(struct inode* buffer, size_t count, uint32_t id);
size_t fs_read_directory_item(struct directory_item* buffer, size_t count, uint32_t id);
size_t fs_read_link(uint32_t* buffer, size_t count, uint32_t id);
size_t fs_read_data(char* buffer, size_t count, uint32_t id);
// write
size_t fs_write_inode(const struct inode* buffer, size_t count, uint32_t id);
size_t fs_write_directory_item(const struct directory_item* buffer, size_t count, uint32_t id);
size_t fs_write_link(const uint32_t* buffer, size_t count, uint32_t id);
size_t fs_write_data(const char* buffer, size_t count, uint32_t id);
// flush
void fs_flush();

#endif
