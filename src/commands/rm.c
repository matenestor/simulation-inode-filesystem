#include <stdio.h>
#include <string.h>

#include "fs_api.h"
#include "iteration_carry.h"
#include "cmd_utils.h"

#include "errors.h"
#include "logger.h"


/*
 * Remove file from filesystem.
 */
int sim_rm(const char* path) {
	log_info("rm: [%s]", path);

	char dir_path[strlen(path) + 1];
	char dir_name[STRLEN_ITEM_NAME] = {0};
	struct inode inode_rm = {0};
	struct inode inode_parent = {0};
	struct carry_dir_item carry = {0};

	// CONTROL

	if (strlen(path) == 0) {
		set_myerrno(Err_arg_missing);
		goto fail;
	}
	// get source's paths and names
	if (split_path(path, dir_path, dir_name) == RETURN_FAILURE) {
		goto fail;
	}
	if (get_inode_wparent(&inode_rm, &inode_parent, path) == RETURN_FAILURE) {
		goto fail;
	}
	if (inode_rm.inode_type != Inode_type_file) {
		set_myerrno(Err_item_not_file);
		goto fail;
	}

	// REMOVE

	carry.id = inode_rm.id_inode;
	strncpy(carry.name, dir_name, STRLEN_ITEM_NAME);

	// delete record from parent
	// if this fails, which should not, only record about the inode is deleted,
	// so it is possible to retrieve it with command 'fsck' --> lost+found/
	if (iterate_links(&inode_parent, &carry, delete_block_item) == RETURN_FAILURE) {
		goto fail;
	}
	if (free_inode_file(&inode_rm) == RETURN_FAILURE) {
		goto fail;
	}

	return RETURN_SUCCESS;

fail:
	log_warning("rm: unable to remove file [%s]", path);
	return RETURN_FAILURE;
}
