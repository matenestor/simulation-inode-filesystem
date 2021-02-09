#include <stdio.h>

#include "fs_cache.h"
#include "errors.h"


/*
 * Remove file from filesystem.
 */
int sim_rm(const char* arg1) {
	int ret = RETURN_FAILURE;
	return ret;
}
//struct carry_dir_item carry = {inode_delete->id_inode, ""};
//struct inode inode_parent = {0};
//
//// delete record from parent
//fs_read_inode(&inode_parent, 1, id_parent);
//iterate_links(&inode_parent, &carry, delete_block_item);
