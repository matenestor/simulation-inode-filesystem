#ifndef INODE_H
#define INODE_H

#include <stdint.h>
#include <stdbool.h>


/** 7 chars (name) + dot + 3 chars (extension) + \0 = 12 chars in total for item name */
#define STRLEN_ITEM_NAME 12

/** Free link in inode. */
#define FREE_LINK -1

// macros for finding out usage of inode
#define isinfree(in) (in->item_type==Item_free)
#define isinfile(in) (in->item_type==Item_file)
#define isindirc(in) (in->item_type==Item_directory)

/** Types of items available in filesystem. */
enum item {
    Item_free,
    Item_file,
    Item_directory
};

struct superblock {
    char signature[16];             // signature of author
    char volume_descriptor[256];    // description of filesystem
    int32_t disk_size;              // total size of filesystem
    int32_t cluster_size;           // cluster size in data part of filesystem
    int32_t cluster_count;          // cluster count in data part of filesystem
    int32_t addr_bm_inodes;         // address of start of i-nodes bitmap
    int32_t addr_bm_data;           // address of start of data bitmap
    int32_t addr_inodes;            // address of start of i-nodes
    int32_t addr_data;              // address of start of data
};

struct inode {
    // meta
    int32_t id_node;                 // i-node id
    enum item item_type;             // type of item in filesystem
    int32_t file_size;               // size of file
    // links
    int32_t direct1;                 // 1. direct link on data clusters
    int32_t direct2;                 // 2. direct link on data clusters
    int32_t direct3;                 // 3. direct link on data clusters
    int32_t direct4;                 // 4. direct link on data clusters
    int32_t direct5;                 // 5. direct link on data clusters
    int32_t indirect1;               // level 1 indirect link on data clusters (pointer-data)
    int32_t indirect2;               // level 2 indirect link on data clusters (pointer-pointer-data)
};

struct directory_item {
    char item_name[STRLEN_ITEM_NAME];   // name of item in directory
    int32_t fk_id_node;                 // i-node of file
};


#endif
