#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include <stdbool.h>


/** 7 chars (name) + dot + 3 chars (extension) + \0 = 12 chars in total for item name */
#define STRLEN_ITEM_NAME 12

/** Free link in inode. */
#define FREE_LINK -1

// it is necessary to increase this values, when more links are added to code
/** Count of direct links in inode. */
#define COUNT_DIRECT_LINKS     5
/** Count of indirect links level 1 in inode. */
#define COUNT_INDIRECT_LINKS_1 1
/** Count of indirect links level 2 in inode. */
#define COUNT_INDIRECT_LINKS_2 1

// macros for finding out usage of inode
#define isinfree(in) (in->item_type==Item_free)
#define isinfile(in) (in->item_type==Item_file)
#define isindirc(in) (in->item_type==Item_directory)

/** Types of items available in filesystem. */
enum item {
    Itemtype_free,
    Itemtype_file,
    Itemtype_directory
};

struct superblock {
    char signature[16];             // signature of author
    char volume_descriptor[256];    // description of filesystem
    int32_t disk_size;              // total size of filesystem in MB
    int32_t cluster_size;           // cluster size in data part of filesystem
    int32_t cluster_count;          // cluster count in data part of filesystem
	size_t count_links;			    // maximum count of indirect links in cluster
	size_t count_dir_items;			// maximum count of directory items in cluster
	int32_t addr_bm_inodes;         // address of start of i-nodes bitmap
    int32_t addr_bm_data;           // address of start of data bitmap
    int32_t addr_inodes;            // address of start of i-nodes
    int32_t addr_data;              // address of start of data
};

struct inode {
    // meta
    int32_t id_inode;                           // i-node id
    enum item item_type;                        // type of item in filesystem
    int32_t file_size;                          // size of file
    int32_t direct[COUNT_DIRECT_LINKS];         // direct links
    int32_t indirect1[COUNT_INDIRECT_LINKS_1];  // indirect links level 1 (pointer-data)
    int32_t indirect2[COUNT_INDIRECT_LINKS_2];  // indirect links level 2 (pointer-pointer-data)
};

struct directory_item {
    char item_name[STRLEN_ITEM_NAME];   // name of item in directory
    int32_t fk_id_inode;                // i-node of file
};

#endif
