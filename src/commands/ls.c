#include <string.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "inode.h"
#include "iteration_carry.h"

#include "logger.h"
#include "errors.h"


/*
 * 	Get inode from given path and list all items inside.
 */
int sim_ls(const char* path) {
	struct inode inode_ls = {0};

	log_info("ls: [%s]", path);

	// no path given -- list actual directory
	if (strlen(path) == 0) {
		memcpy(&inode_ls, &in_actual, sizeof(struct inode));
	}
	// else get last inode in path
	else {
		get_inode(&inode_ls, path);
	}

	switch (inode_ls.inode_type) {
		case Inode_type_dirc:
			iterate_links(&inode_ls, NULL, list_items);
			break;

		case Inode_type_file:
			puts(path);
			break;

		default:
			set_myerrno(Err_item_not_exists);
			log_warning("ls: unable to list [%s]", path);
			return RETURN_FAILURE;
	}

	return RETURN_SUCCESS;
}
