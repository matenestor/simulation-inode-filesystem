#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "inode.h"
#include "cmd_utils.h"

#include "logger.h"
#include "errors.h"


/*
 * Copy file from normal filesystem into simulation filesystem.
 */
int sim_incp(const char* path_source, const char* path_target) {
	log_info("incp: [%s] [%s]", path_source, path_target);

	struct stat st = {0};
	FILE* f_source = NULL;
	uint32_t count_blocks = 0;
	uint32_t count_existing_block = 0;
	uint32_t count_empty_blocks = 0;
	uint32_t* links = NULL;
	char dir_path[strlen(path_target) + 1];
	char dir_name[STRLEN_ITEM_NAME] = {0};
	struct inode inode_target = {0};
	struct inode inode_parent = {0};
	struct carry_stream carry_stream = {0};
	struct carry_dir_item carry_dir = {0};

	// CONTROL

	if (strlen(path_source) == 0 || strlen(path_target) == 0) {
		set_myerrno(Err_arg_missing);
		goto fail;
	}
	if (split_path(path_target, dir_path, dir_name) == RETURN_FAILURE) {
		goto fail;
	}
	// 'path_source' file exists in OS
	if (access(path_source, F_OK) != 0) {
		set_myerrno(Err_item_not_exists);
		goto fail;
	}
	// open file to load data from
	if (stat(path_source, &st) == -1
			|| (f_source = fopen(path_source, "rb")) == NULL) {
		set_myerrno(Err_os_open_file);
		log_error("Unable to open system file [%s].", path_source);
		goto fail;
	}
	// count of data blocks with data (only -- no deep blocks of indirect links)
	count_blocks = get_count_data_blocks(st.st_size);
	if (count_blocks >= sb.block_count
		|| count_blocks * sb.block_size > sb.max_file_size) {
		set_myerrno(Err_os_file_too_big);
		goto fail;
	}
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

	// IN-COPY

	// inode to copy file into, already exists in filesystem
	if (get_inode(&inode_target, path_target) != RETURN_FAILURE) {
		// rewrite only files
		if (inode_target.inode_type != Inode_type_file) {
			set_myerrno(Err_item_not_file);
			goto fail;
		}

		// current amount of data block, that inode is pointing at
		count_existing_block = get_count_data_blocks(inode_target.file_size);

		// create missing blocks
		if (count_blocks > count_existing_block) {
			create_empty_links(links, count_blocks - count_existing_block, &inode_target);
		}
		// remove additional blocks
		else if (count_blocks < count_existing_block) {
			free_amount_of_links(&inode_target, count_existing_block - count_blocks);
		}

		// rewrite
		carry_stream.file = f_source;
		iterate_links(&inode_target, &carry_stream, incp_data);
		update_size(&inode_target, st.st_size);
	}
	// create inode to copy file into, exists in filesystem
	else if (create_inode_file(&inode_target) != RETURN_FAILURE) {
		// create links in new inode to data blocks
		if (create_empty_links(links, count_blocks, &inode_target) == RETURN_FAILURE) {
			free_inode_file(&inode_target);
			goto fail;
		}
		// get parent of new file -- its path is new file's 'dir_path'
		if (get_inode(&inode_parent, dir_path) == RETURN_FAILURE) {
			goto fail;
		}
		// add new inode to parent
		carry_dir.id = inode_target.id_inode;
		strncpy(carry_dir.name, dir_name, strlen(dir_name));
		if (add_to_parent(&inode_parent, &carry_dir) == RETURN_FAILURE) {
			free_inode_directory(&inode_target);
			goto fail;
		}

		// copy data
		incp_data_inplace(links, count_blocks, f_source);
		update_size(&inode_target, st.st_size);
	}
	else {
		goto fail;
	}

	free(links);
	fclose(f_source);
	// can be set during 'get_inode()', when checking if file exists
	reset_myerrno();
	return RETURN_SUCCESS;

fail:
	if (links != NULL)
		free(links);
	if (f_source != NULL)
		fclose(f_source);
	log_warning("incp: unable to incopy file [%s] [%s]", path_source, path_target);
	return RETURN_FAILURE;
}
