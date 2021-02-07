#include <stdint.h>
#include <string.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "inode.h"

#include "logger.h"
#include "errors.h"


/*
 * 	Check if making new directory is possible by reading parent of the directory
 * 	and then trying to read inode with name of the directory.
 */
static bool is_mkdir_possible(const char* path, const char* path_parent) {
	bool is_creatable = false;
	struct inode in_tmp = {0};

	// path to destination inode exists
	if (get_inode_by_path(&in_tmp, path_parent) != RETURN_FAILURE) {
		// no directory with same name as new being created exists
		if (get_inode_by_path(&in_tmp, path) == RETURN_FAILURE) {
			is_creatable = true;
		}
		else {
			set_myerrno(Err_dir_exists);
		}
	}

	return is_creatable;
}

/*
 * 	Makes new directory. Check if path was given, parse name from path,
 * 	check if directory creation is possible, get inode where the directory will be created,
 * 	get free link in the inode, create new inode for the directory.
 * 	If everything above is successful, initialize new directory and make record of it
 * 	in parent inode and data block.
 *
 */
int sim_mkdir(const char* path) {
	int ret = RETURN_FAILURE;

	log_info("mkdir: creating [%s]", path);

	// count of records in block read
	size_t items = 0;
	// name of new directory
	char dir_name[STRLEN_ITEM_NAME] = {0};
	// path to parent of new directory, if any
	char path_parent[strlen(path) + 1];
	// id of block, where record of new directory will be stored
	int32_t id_block = RETURN_FAILURE;
	// parent of directory being created
	struct inode in_parent = {0};
	// inode of new directory
	struct inode in_new_dir = {0};
	// struct of new directory, which will be put to block
	struct directory_item new_dir = {0};
	// block of directory records, where the new record will be stored
	struct directory_item dirs[sb.count_dir_items];

	// separate path to parent directory from new directory name
	my_dirname(path_parent, path);

	if (strlen(path) > 0) {
		// get name -- last element in path
		if (my_basename(dir_name, path, STRLEN_ITEM_NAME) != RETURN_FAILURE) {
			// check if it is possible to make new directory
			if (is_mkdir_possible(path, path_parent)) {
				// get parent inode, where new directory should be created in
				if (get_inode_by_path(&in_parent, path_parent) != RETURN_FAILURE) {
					// get link to block in parent, where new directory will be written to
					if ((id_block = get_empty_link(&in_parent, 1)) != RETURN_FAILURE) {
						// create inode for new directory record
						if (create_inode_directory(&in_new_dir, in_parent.id_inode) != RETURN_FAILURE) {
							// cache block where the record of new directory will be stored
							fs_read_directory_item(dirs, sb.count_dir_items, id_block);
//							items = get_count_dirs(dirs);

							// init new directory
							strncpy(new_dir.item_name, dir_name, strlen(dir_name) + 1);
							new_dir.fk_id_inode = in_new_dir.id_inode;

							// insert new dir to the block
							dirs[items] = new_dir;

							// write updated block
							fs_write_directory_item(dirs, items + 1, id_block);

							fs_flush();

							ret = RETURN_SUCCESS;
						}
						// TODO longterm:
						//  else delete the link for 'id_block' if it is empty
					}
				}
			}
		}
	}
	else {
		set_myerrno(Err_arg_missing);
		log_warning("mkdir: unable to make directory [%s]", path);
	}

	return ret;
}
