#include <stdint.h>
#include <string.h>

#include "fs_api.h"
#include "inode.h"
#include "iteration_carry.h"
#include "cmd_utils.h"

#include "logger.h"
#include "errors.h"


/*
 * Make new directory in filesystem.
 */
int sim_mkdir(const char* path) {
	log_info("mkdir: creating [%s]", path);

	char dir_path[strlen(path) + 1];
	char dir_name[STRLEN_ITEM_NAME] = {0};
	struct inode inode_parent = {0};
	struct inode inode_new_dir = {0};
	struct carry_directory_item carry = {0};
	// link number to empty block, in case all blocks of parent are full
	uint32_t empty_block[1] = {0};

	if (strlen(path) == 0) {
		set_myerrno(Err_arg_missing);
		goto fail;
	}
	if (split_path(path, dir_path, dir_name) == RETURN_FAILURE) {
		goto fail;
	}
	// get parent inode, where new directory should be created in
	if (get_inode(&inode_parent, dir_path) == RETURN_FAILURE) {
		goto fail;
	}
	// check if item already exists in parent
	if (item_exists(&inode_parent, dir_name)) {
		set_myerrno(Err_item_exists);
		goto fail;
	}
	// create inode for new directory record
	if (create_inode_directory(&inode_new_dir, inode_parent.id_inode) == RETURN_FAILURE) {
		goto fail;
	}

	carry.id = inode_new_dir.id_inode;
	strncpy(carry.name, dir_name, strlen(dir_name));

	// add record to parent inode about new directory
	if (iterate_links(&inode_parent, &carry, add_block_item) == RETURN_FAILURE) {
		// parent inode has all, so far created, blocks full
		if (create_empty_links(empty_block, 1, &inode_parent) != RETURN_FAILURE) {
			init_block_with_directories(empty_block[0]);
			add_block_item(empty_block, 1, &carry);
		} // error while creating new link, or parent inode is completely full of directories
		else {
			free_inode_directory(&inode_new_dir);
			goto fail;
		}
	}

	return RETURN_SUCCESS;

fail:
	log_warning("mkdir: unable to make directory [%s]", path);
	return RETURN_FAILURE;
}
