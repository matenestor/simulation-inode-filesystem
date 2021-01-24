#include <stdio.h>
#include <string.h>

#include "../fs_operations.h"
#include "inode.h"

#include "../../include/logger.h"
#include "../../include/errors.h"


int outcp_(const char* source, const char* target) {
	int ret = RETURN_FAILURE;
	struct inode in_source = {0};
	FILE* f_target = NULL;

	if (strlen(source) > 0 && strlen(target) > 0) {
		if (get_inode_by_path(&in_source, target) != RETURN_FAILURE) {
			if ((f_target = fopen(target, "wb")) != NULL) {

			}
		}
	}
	else {
		set_myerrno(Err_arg_missing);
		log_warning("outcp: unable to outcopy file [%s] [%s]", source, target);
	}

	return ret;
}

