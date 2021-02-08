#include <stdint.h>
#include <string.h>
#include <libgen.h>

#include "fs_api.h"
#include "inode.h"
#include "iteration_carry.h"

#include "logger.h"
#include "errors.h"


/*
 * Split given path to path to last element and item name there.
 *  "/usr/local/bin/inodes" --> "/usr/local/bin" + "inodes"
 */
static int split_path(const char* path, char* const dir_path, char* const dir_name) {
	size_t path_length = strlen(path) + 1;
	char copy_dirname[path_length];
	char copy_basename[path_length];
	char* p_copy_dirname = NULL;
	char* p_copy_basename = NULL;

	// copies for dirname() and basename()
	strncpy(copy_dirname, path, path_length);
	strncpy(copy_basename, path, path_length);
	// get pointer to elements
	p_copy_dirname = dirname(copy_dirname);
	p_copy_basename = basename(copy_basename);

	if (strlen(p_copy_basename) < STRLEN_ITEM_NAME) {
		// copy dir path and name
		strncpy(dir_path, p_copy_dirname, strlen(p_copy_dirname) + 1);
		strncpy(dir_name, p_copy_basename, strlen(p_copy_basename) + 1);
		return RETURN_SUCCESS;
	} // new directory name is too long
	else {
		set_myerrno(Err_item_name_long);
		return RETURN_FAILURE;
	}
}

/*
 * 	Check if making new directory is possible.
 */
static bool item_exists(const struct inode* inode_parent, const char* dir_name) {
	bool exists = false;
	struct carry_directory_item carry = {0};

	carry.id = FREE_LINK;
	strncpy(carry.name, dir_name, STRLEN_ITEM_NAME);

	// check if item already exists
	if (iterate_links(inode_parent, &carry, search_block_inode_id) != RETURN_FAILURE) {
		exists = true;
	}
	return exists;
}

/*
 * Make new directory in filesystem.
 */
int sim_mkdir(const char* path) {
	log_info("mkdir: creating [%s]", path);

	char dir_path[strlen(path) + 1];		// path to parent of new directory
	char dir_name[STRLEN_ITEM_NAME] = {0};	// name of new directory
	uint32_t empty_block[1] = {0};			// link number to empty block,
											// in case all blocks of parent are full
	struct inode inode_parent = {0};		// parent of directory being created
	struct inode inode_new_dir = {0};		// inode of new directory
	struct carry_directory_item carry = {0};

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
