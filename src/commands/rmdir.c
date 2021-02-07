#include <stdint.h>
#include <string.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "inode.h"

#include "logger.h"
#include "errors.h"


static int check_dot_dirs(const char* dir_name) {
	int ret = RETURN_FAILURE;

	if (strcmp(".", dir_name) == 0) {
		set_myerrno(Err_dir_arg_invalid);
	}
	else if (strcmp("..", dir_name) == 0) {
		set_myerrno(Err_dir_not_empty);
	}
	else {
		ret = RETURN_SUCCESS;
	}

	return ret;
}

static bool is_array_empty(const int32_t* array, const size_t size, const size_t begin) {
	bool ret = true;
	size_t i;

	for (i = begin; i < size; ++i) {
		if (array[i] != FREE_LINK) {
			ret = false;
			break;
		}
	}

	return ret;
}

static int is_dir_empty(const struct inode* in_target) {
	int ret = RETURN_FAILURE;

	if (is_array_empty(in_target->direct, COUNT_DIRECT_LINKS, 1)
			&& is_array_empty(in_target->indirect_1, COUNT_INDIRECT_LINKS_1, 0)
			&& is_array_empty(in_target->indirect_2, COUNT_INDIRECT_LINKS_2, 0)) {
		ret = RETURN_SUCCESS;
	}
	else {
		set_myerrno(Err_dir_not_empty);
	}

	return ret;
}

static int is_block_empty(const int32_t id_block) {
	int ret = RETURN_FAILURE;
	size_t items = 0;
	struct directory_item block[sb.count_dir_items];

	fs_read_directory_item(block, sb.count_dir_items, id_block);
//	items = get_count_dirs(block);

	// in directory are only "." and ".." directories
	if (items == 2) {
		ret = RETURN_SUCCESS;
	}
	else if (items > 2){
		set_myerrno(Err_dir_not_empty);
	}
	// never should get here, unless manually changing bytes in raw fs file
	else {
		set_myerrno(Err_fs_error);
	}

	return ret;
}

int sim_rmdir(const char* path) {
	int ret = RETURN_FAILURE;
	char dir_name[STRLEN_ITEM_NAME] = {0};
	struct inode in_rmdir = {0};

	if (strlen(path) > 0) {
		if (my_basename(dir_name, path, STRLEN_ITEM_NAME) != RETURN_FAILURE) {
			if (check_dot_dirs(dir_name) != RETURN_FAILURE) {
				if (get_inode_by_path(&in_rmdir, path) != RETURN_FAILURE) {
					if (in_rmdir.inode_type == Inode_type_dirc) {
						if (is_dir_empty(&in_rmdir) != RETURN_FAILURE) {
							if (is_block_empty(in_rmdir.direct[0]) != RETURN_FAILURE) {
//								ret = destroy_inode(&in_rmdir);
							}
						}
					}
					else {
						set_myerrno(Err_item_not_directory);
					}
				}
			}
		}
	}
	else {
		set_myerrno(Err_arg_missing);
	}

	if (ret == RETURN_FAILURE) {
		log_warning("rmdir: unable to remove directory [%s]", path);
	}

	return ret;
}
