#include <stdio.h>
#include <string.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "fs_prompt.h"
#include "inode.h"

#include "logger.h"
#include "errors.h"


static int handle_prompt_change() {
	// pwd change
	char new_pwd[STRLEN_PWD_LENGTH] = {0};
	get_path_to_root(new_pwd, STRLEN_PWD_LENGTH, &in_actual);
	strncpy(buff_pwd, new_pwd, STRLEN_PWD_LENGTH);
	// whole prompt change
	snprintf(buff_prompt, BUFFER_PROMPT_LENGTH, FORMAT_PROMPT, fs_name, buff_pwd);
	return 0;
}

/*
 * Change directory in filesystem.
 */
int sim_cd(const char* path) {
	// +2 if path == 0, then SEPARATOR and \0 will be copied
	size_t path_length = strlen(path);
	char target[path_length + 2];
	struct inode inode_cd;

	log_info("cd: [%s]", path);

    // target is either path, or root, when no path is given
	if (path_length > 0) {
		strncpy(target, path, path_length + 1);
	} else {
		strncpy(target, SEPARATOR, 2);
	}

	if (get_inode(&inode_cd, target) == RETURN_FAILURE) {
		set_myerrno(Err_item_not_exists);
		goto fail;
	}
	if (inode_cd.inode_type != Inode_type_dirc) {
		set_myerrno(Err_item_not_directory);
		goto fail;
	}

	// change directory
	memcpy(&in_actual, &inode_cd, sizeof(struct inode));
	handle_prompt_change();

	return RETURN_SUCCESS;

fail:
	log_warning("cd: unable to change directory to [%s]", path);
	return RETURN_FAILURE;
}
