#include <string.h>
#include <libgen.h>

#include "cmd_utils.h"
#include "inode.h"

#include "errors.h"


/*
 * Split given path to path to last element and item name there.
 *  "/usr/local/bin/inodes" --> "/usr/local/bin" + "inodes"
 */
int split_path(const char* path, char* const dir_path, char* const dir_name) {
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
