#include <stdio.h>
#include <string.h>

#include "fs_api.h"
#include "inode.h"
#include "iteration_carry.h"
#include "cmd_utils.h"

#include "errors.h"
#include "logger.h"

/*
 * Move file in filesystem from given directory to another directory.
 */
int sim_mv(const char* path_source, const char* path_destination) {
	log_info("mv: [%s] [%s]", path_source, path_destination);

	char dir_path_src[strlen(path_source) + 1];
	char dir_name_src[STRLEN_ITEM_NAME] = {0};
	char dir_path_dest[strlen(path_destination) + 1];
	char dir_name_dest[STRLEN_ITEM_NAME] = {0};
	struct inode inode_src = {0};			// inode to move
	struct inode inode_src_parent = {0};	// parent directory of inode to move
	struct inode inode_dest = {0};			// destination directory of inode to move
	struct carry_dir_item carry_dir = {0};

	// CONTROL

	if (strlen(path_source) == 0 || strlen(path_destination) == 0) {
		set_myerrno(Err_arg_missing);
		goto fail;
	}
	// get source's and destination's paths and names
	if (split_path(path_source, dir_path_src, dir_name_src) == RETURN_FAILURE
		|| split_path(path_destination, dir_path_dest, dir_name_dest) == RETURN_FAILURE) {
		goto fail;
	}

	// get source inode
	if (get_inode_wparent(&inode_src, &inode_src_parent, path_source) == RETURN_FAILURE) {
		goto fail;
	}

	// get last two inodes in path
	// 1: last element in path is directory, where inode will moved to
	if (get_inode(&inode_dest, path_destination) != RETURN_FAILURE) {
		strncpy(carry_dir.name, dir_name_src, STRLEN_ITEM_NAME); // name of file after move
	}
	// 2: last element in path is name of moved file
	else if (get_inode(&inode_dest, dir_path_dest) != RETURN_FAILURE) {
		strncpy(carry_dir.name, dir_name_dest, STRLEN_ITEM_NAME); // name of file after move
	}
	// loading failed -- destination path doesn't exist
	else {
		goto fail;
	}

	// destination inode must be directory, but if loaded 'inode_dest' is file,
	// then some item already exists there
	if (inode_dest.inode_type == Inode_type_file) {
		set_myerrno(Err_item_exists);
		goto fail;
	}
	// else it is really a directory, so check if item exists in destination already
	if (iterate_links(&inode_dest, &carry_dir, search_block_inode_id) != RETURN_FAILURE) {
		set_myerrno(Err_item_exists);
		goto fail;
	}

	// MOVE

	carry_dir.id = inode_src.id_inode; // id of moved file
	// move item record to another inode (or same one, just with different name)
	if (add_to_parent(&inode_dest, &carry_dir) == RETURN_FAILURE) {
		goto fail;
	}
	// delete moved record from former place
	strncpy(carry_dir.name, dir_name_src, STRLEN_ITEM_NAME);
	iterate_links(&inode_src_parent, &carry_dir, delete_block_item);

	// can be set during 'get_inode()', when checking if file exists
	reset_myerrno();
	return RETURN_SUCCESS;

fail:
	log_warning("mv: unable to copy file [%s] [%s]", path_source, path_destination);
	return RETURN_FAILURE;
}
