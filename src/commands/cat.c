#include <string.h>

#include "fs_api.h"
#include "inode.h"
#include "iteration_carry.h"

#include "errors.h"
#include "logger.h"


/*
 * Concatenate file from filesystem and print it to standard output.
 */
int sim_cat(const char* path_source) {
	log_info("cat: [%s]", path_source);

	struct inode inode_source = {0};

	// CONTROL

	if (strlen(path_source) == 0) {
		set_myerrno(Err_arg_missing);
		goto fail;
	}
	if (get_inode(&inode_source, path_source) == RETURN_FAILURE) {
		goto fail;
	}
	if (inode_source.inode_type != Inode_type_file) {
		set_myerrno(Err_item_not_file);
		goto fail;
	}

	// CONCATENATE

	iterate_links(&inode_source, NULL, cat_data);

	return RETURN_SUCCESS;

fail:
	log_warning("cat: unable to concatenate file [%s]", path_source);
	return RETURN_FAILURE;
}
