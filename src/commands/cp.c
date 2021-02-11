#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs_api.h"
#include "inode.h"
#include "iteration_carry.h"
#include "cmd_utils.h"

#include "errors.h"
#include "logger.h"


/*
 * Copy file in filesystem.
 */
int sim_cp(const char* path_source, const char* path_destination) {
	log_info("cp: [%s] [%s]", path_source, path_destination);

	char dir_path_src[strlen(path_source) + 1];
	char dir_name_src[STRLEN_ITEM_NAME] = {0};
	char dir_path_dest[strlen(path_destination) + 1];
	char dir_name_dest[STRLEN_ITEM_NAME] = {0};
	uint32_t* links = NULL;
	uint32_t count_blocks = 0;
	uint32_t count_empty_blocks = 0;
	struct inode inode_new = {0};		// new inode to copy data into
	struct inode inode_src = {0};		// inode to copy
	struct inode inode_dest = {0};		// destination directory of inode to copy
	struct carry_copy carry_copy = {0};
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
	if (get_inode(&inode_src, path_source) == RETURN_FAILURE) {
		goto fail;
	}
	// check if source inode is file
	if (inode_src.inode_type != Inode_type_file) {
		set_myerrno(Err_item_not_file);
		goto fail;
	}

	// count of data blocks with data (only -- no deep blocks of indirect links)
	count_blocks = get_count_data_blocks(inode_src.file_size);
	// count of empty data blocks in filesystem
	count_empty_blocks = get_empty_fields_amount_data();
	if (is_error()) { // error during malloc in 'get_empty_fields_amount_data()'
		goto fail;
	}
	// check enough blocks and space in filesystem
	if (count_empty_blocks == 0 || !is_enough_space(count_blocks, count_empty_blocks)) {
		set_myerrno(Err_block_no_blocks);
		goto fail;
	}

	if ((links = malloc(count_blocks * sizeof(uint32_t))) == NULL) {
		set_myerrno(Err_malloc);
		goto fail;
	}

	// get last two inodes in path
	// 1: last element in path is directory, where inode will copied to
	if (get_inode(&inode_dest, path_destination) != RETURN_FAILURE) {
		strncpy(carry_dir.name, dir_name_src, STRLEN_ITEM_NAME); // name of file after copy
	}
	// 2: last element in path is name of copied file
	else if (get_inode(&inode_dest, dir_path_dest) != RETURN_FAILURE) {
		strncpy(carry_dir.name, dir_name_dest, STRLEN_ITEM_NAME); // name of file after copy
	}
	// loading failed -- destination path doesn't exist
	else {
		goto fail;
	}

	// NOTE: here it is possible to compare inode ids, if 'inode_dest' is a file
	//  and if they are same -- notify about same inode, or ask for rewrite,
	//  if some inode is found (create new buffer for the string concat)

	// destination inode must be directory, but if loaded 'inode_dest' is file,
	// then some item already exists there
	if (inode_dest.inode_type == Inode_type_file) {
		set_myerrno(Err_item_exists);
		goto fail;
	}
	// check if item exists in destination file already, if yes, then error
	if (iterate_links(&inode_dest, &carry_dir, search_block_inode_id) != RETURN_FAILURE) {
		set_myerrno(Err_item_exists);
		goto fail;
	}

	// COPY

	// create inode to copy into
	if (create_inode_file(&inode_new) == RETURN_FAILURE) {
		goto fail;
	}
	if (create_empty_links(links, count_blocks, &inode_new) == RETURN_FAILURE) {
		free_inode_file(&inode_new);
		goto fail;
	}

	// add new inode to destination parent directory
	carry_dir.id = inode_new.id_inode; // id of new copied file
	if (add_to_parent(&inode_dest, &carry_dir) == RETURN_FAILURE) {
		free_inode_file(&inode_new);
		goto fail;
	}

	carry_copy.dest_links = links;
	carry_copy.links_count = count_blocks;

	// copy date from source inode to newly created inode -- use newly created links in the inode
	if (iterate_links(&inode_src, &carry_copy, copy_data) == RETURN_FAILURE) {
		free_inode_file(&inode_new);
		goto fail;
	}
	update_size(&inode_new, inode_src.file_size);

	free(links);
	// can be set during 'get_inode()', when checking if path exists
	reset_myerrno();
	return RETURN_SUCCESS;

fail:
	if (links != NULL)
		free(links);
	log_warning("cp: unable to copy file [%s] [%s]", path_source, path_destination);
	return RETURN_FAILURE;
}
