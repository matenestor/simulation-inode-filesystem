#ifndef FS_API_H
#define FS_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "inode.h"
#include "iteration_carry.h"


// FILESYSTEM COMMON FUNCTIONS

int init_filesystem(const char* fsp, bool* is_formatted);
void close_filesystem();
uint32_t get_count_data_blocks(const off_t file_size);
bool is_enough_space(const uint32_t count_blocks, const uint32_t count_empty_blocks);

// FILESYSTEM BITMAP FUNCTIONS

uint32_t allocate_bitmap_field_inode();
uint32_t allocate_bitmap_field_data();
void free_bitmap_field_inode(int32_t id);
void free_bitmap_field_data(int32_t id);
uint32_t get_empty_fields_amount_data();
void read_whole_bitmap_inodes(bool* bitmap);
void read_whole_bitmap_data(bool* bitmap);

// FILESYSTEM INODE FUNCTIONS

int free_inode_file(struct inode* id_inode);
int free_inode_directory(struct inode* id_inode);
uint32_t create_inode_file(struct inode* new_inode);
uint32_t create_inode_directory(struct inode* new_inode, const uint32_t id_parent);

// FILESYSTEM LINK FUNCTIONS

int free_amount_of_links(struct inode* inode_target, const size_t to_free);
int create_empty_links(uint32_t* buffer, const size_t to_create, struct inode* inode_source);

// FILESYSTEM DATA BLOCK FUNCTIONS

int init_block_with_directories(const uint32_t id_block);
int init_empty_dir_block(struct directory_item* block, const uint32_t id_self, const uint32_t id_parent);
int incp_data_inplace(const uint32_t* links, const uint32_t links_count, FILE* file);

// FILESYSTEM UTILS FUNCTIONS

int get_inode(struct inode* inode_dest, const char* path);
int get_inode_wparent(struct inode* inode_dest, struct inode* inode_parent, const char* path);
int get_path_to_root(char* dest_path, const size_t length_path, const struct inode* inode_source);
int add_to_parent(struct inode* inode_parent, struct carry_dir_item* carry);
bool is_directory_empty(const struct inode* inode_source);
bool item_exists(const struct inode* inode_parent, const char* dir_name);
int update_size(struct inode* inode_target, const uint32_t file_size);
int load_inode_ids(bool* inode_ids, const size_t ids_count, int* active);

// FILESYSTEM INPUT/OUTPUT FUNCTIONS

// read
size_t fs_read_inode(struct inode* buffer, const size_t count, const uint32_t id);
size_t fs_read_directory_item(struct directory_item* buffer, const size_t count, const uint32_t id);
size_t fs_read_link(uint32_t* buffer, const size_t count, const uint32_t id);
size_t fs_read_data(char* buffer, const size_t count, const uint32_t id);
// write
size_t fs_write_inode(const struct inode* buffer, const size_t count, const uint32_t id);
size_t fs_write_directory_item(const struct directory_item* buffer, const size_t count, const uint32_t id);
size_t fs_write_link(const uint32_t* buffer, const size_t count, const uint32_t id);
size_t fs_write_data(const char* buffer, const size_t count, const uint32_t id);
// flush
void fs_flush();

#endif
