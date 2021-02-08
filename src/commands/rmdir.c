#include <string.h>

#include "fs_api.h"
#include "inode.h"
#include "iteration_carry.h"
#include "cmd_utils.h"

#include "logger.h"
#include "errors.h"


/*
 * Remove directory from filesystem, if it is not empty.
 */
int sim_rmdir(const char* path) {
	log_info("rmdir: removing [%s]", path);

	char dir_path[strlen(path) + 1];
	char dir_name[STRLEN_ITEM_NAME] = {0};
	struct inode inode_parent = {0};
	struct inode inode_rmdir = {0};
	struct carry_directory_item carry = {0};

	if (strlen(path) == 0) {
		set_myerrno(Err_arg_missing);
		goto fail;
	}
	if (split_path(path, dir_path, dir_name) == RETURN_FAILURE) {
		goto fail;
	}
	if (strcmp(dir_name, ".") == 0) {
		set_myerrno(Err_dir_arg_invalid);
		goto fail;
	}
	// load inode to remove and its parent for removing record of the directory
	if (get_inode(&inode_rmdir, path) == RETURN_FAILURE
		|| get_inode(&inode_parent, dir_path) == RETURN_FAILURE) {
		goto fail;
	}

	carry.id = inode_rmdir.id_inode;
	strncpy(carry.name, dir_name, STRLEN_ITEM_NAME);

	// delete directory itself
	if (free_inode_directory(&inode_rmdir) == RETURN_FAILURE) {
		goto fail;
	}
	// delete record from parent
	if (iterate_links(&inode_parent, &carry, delete_block_item) == RETURN_FAILURE) {
		goto fail;
	}

	return RETURN_SUCCESS;

fail:
	log_warning("rmdir: unable to remove directory [%s]", path);
	return RETURN_FAILURE;
}
