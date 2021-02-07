#include <string.h>
#include <unistd.h>

#include "fs_api.h"
#include "inode.h"

#include "logger.h"
#include "errors.h"


int sim_incp(const char* source, const char* target) {
	int ret = RETURN_FAILURE;
	struct inode in_target = {0};

	if (strlen(source) > 0 && strlen(target) > 0) {
		// 'source' file exists in OS
		if (access(source, F_OK) == 0) {
			// inode, to copy file into, exists in filesystem
			if (get_inode_by_path(&in_target, target) != RETURN_FAILURE) {
				// todo fopen 'source' file and copy into inode
			}
			else if (create_inode_file(&in_target)) {
				// todo GET ID OF PARENT
				// todo fopen 'source' file and copy into inode
			}
			else {
				// todo error
			}
		}
	}
	else {
		set_myerrno(Err_arg_missing);
		log_warning("incp: unable to incopy file [%s] [%s]", source, target);
	}

	return ret;
}
