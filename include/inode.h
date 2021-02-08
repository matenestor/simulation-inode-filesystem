#ifndef INODE_H
#define INODE_H

#include <stdint.h>

// 7 chars (name) + dot + 3 chars (extension) + \0 =
// = 12 chars in total for item name
#define STRLEN_ITEM_NAME 		12

#define ROOT_ID					1

#define FREE_LINK 				0
#define COUNT_DIRECT_LINKS		5
#define COUNT_INDIRECT_LINKS_1	1
#define COUNT_INDIRECT_LINKS_2	1

// types of inodes in filesystem
enum item {
	Inode_type_free,	// free, not occupied
	Inode_type_file,	// file inode
	Inode_type_dirc,	// directory inode
};

struct superblock {
	char signature[16];				// signature of author
	char volume_descriptor[256];	// description of filesystem
	uint32_t disk_size;				// total size of filesystem in MB
	uint32_t block_size;			// block size in data part of filesystem
	uint32_t block_count;			// block count in data part of filesystem
	uint32_t count_links;			// maximum count of indirect links in block
	uint32_t count_dir_items;		// maximum count of directory items in block
	uint32_t addr_bm_inodes;		// address of start of i-nodes bitmap
	uint32_t addr_bm_data;			// address of start of data bitmap
	uint32_t addr_inodes;			// address of start of i-nodes
	uint32_t addr_data;				// address of start of data
};

struct inode {
	uint32_t id_inode;								// i-node id
	enum item inode_type;							// type of item in filesystem
	uint32_t file_size;								// size of file
	uint32_t direct[COUNT_DIRECT_LINKS];			// direct links
	uint32_t indirect_1[COUNT_INDIRECT_LINKS_1];	// indirect links level 1 (pointer-data)
	uint32_t indirect_2[COUNT_INDIRECT_LINKS_2];	// indirect links level 2 (pointer-pointer-data)
};

struct directory_item {
	uint32_t fk_id_inode;				// i-node of file
	char item_name[STRLEN_ITEM_NAME];	// name of item in directory
};

#endif

/*
def max_filesize(d, in1, in2, block):
    cl = block // 4
    x = (d + in1*cl + in2*cl*cl) * block
    print(f'{x} B\n{x//1024} KB\n{x//1024//1024} MB')
*/
