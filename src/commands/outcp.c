#include <stdio.h>
#include <string.h>

#include "fs_api.h"
#include "inode.h"
#include "iteration_carry.h"

#include "logger.h"
#include "errors.h"


/*
 * Copy file from simulation filesystem outside to normal filesystem.
 */
int sim_outcp(const char* path_source, const char* path_target) {
	struct inode inode_source = {0};
	FILE* f_target = NULL;
	struct carry_stream carry = {0};

	// CONTROL

	if (strlen(path_source) == 0 || strlen(path_target) == 0) {
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
	if ((f_target = fopen(path_target, "wb")) == NULL) {
		set_myerrno(Err_os_open_file);
		log_error("Unable to open system file [%s].", path_target);
		goto fail;
	}

	// OUT-COPY

	carry.file = f_target;
	carry.data_count = inode_source.file_size;
	iterate_links(&inode_source, &carry, outcp_data);

	fclose(f_target);
	return RETURN_SUCCESS;

fail:
	if (f_target != NULL)
		fclose(f_target);
	log_warning("outcp: unable to outcopy file [%s] [%s]", path_source, path_target);
	return RETURN_FAILURE;
}
