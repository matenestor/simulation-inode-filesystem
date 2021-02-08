#include <string.h>
#include <libgen.h>

#include "fs_api.h"
#include "inode.h"

#include "logger.h"
#include "errors.h"


/*
 * Remove directory from filesystem, if it is not empty.
 */
int sim_rmdir(const char* path) {
	char path_copy[strlen(path) + 1];
	char* p_basename = NULL;
	struct inode inode_rmdir = {0};

	if (strlen(path) == 0) {
		set_myerrno(Err_arg_missing);
		goto fail;
	}

	strncpy(path_copy, path, strlen(path) + 1);
	p_basename = basename(path_copy);

	if (strcmp(p_basename, ".") == 0) {
		set_myerrno(Err_dir_arg_invalid);
		goto fail;
	}
	if (get_inode(&inode_rmdir, path) == RETURN_FAILURE) {
		goto fail;
	}
	if (free_inode_directory(&inode_rmdir) == RETURN_FAILURE) {
		goto fail;
	}

	return RETURN_SUCCESS;

fail:
	log_warning("rmdir: unable to remove directory [%s]", path);
	return RETURN_FAILURE;
}
