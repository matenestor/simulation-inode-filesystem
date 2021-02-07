#include <stdio.h>
#include <string.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "fs_prompt.h"
#include "inode.h"

#include "logger.h"
#include "errors.h"


static int handle_pwd_change() {
    char new_pwd[STRLEN_PWD_LENGTH] = {0};
    get_path_to_root(new_pwd, STRLEN_PWD_LENGTH);
    strncpy(buff_pwd, new_pwd, STRLEN_PWD_LENGTH);
    return 0;
}

static int handle_prompt_change() {
    handle_pwd_change();
    snprintf(buff_prompt, BUFFER_PROMPT_LENGTH, FORMAT_PROMPT, fs_name, buff_pwd);
    return 0;
}

int sim_cd(const char* path) {
    int ret = RETURN_FAILURE;
    // +2 if path == 0, then SEPARATOR and \0 will be copied
	size_t len = strlen(path);
    char target[len + 2];
    struct inode in_cd;
    uint32_t id_cd, id_parent;

    log_info("cd: [%s]", path);

    // target is either path, or root, when no path is given
    if (len > 0) {
		strncpy(target, path, len + 1);
    } else {
		strncpy(target, SEPARATOR, 2);
    }

    if (get_inode_by_path(target, &id_cd, &id_parent) != RETURN_FAILURE) {
    	fs_read_inode(&in_cd, 1, id_cd);
        // change directory
        memcpy(&in_actual, &in_cd, sizeof(struct inode));

        handle_prompt_change();

        ret = RETURN_SUCCESS;
    }
    else {
        set_myerrno(Err_item_not_exists);
        log_warning("cd: unable to change directory to [%s]", path);
    }

    return ret;
}
