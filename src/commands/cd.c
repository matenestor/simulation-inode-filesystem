#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/fs_cache.h"
#include "../inc/fs_prompt.h"
#include "../inc/inode.h"
#include "../inc/return_codes.h"
#include "../fs_operations.h"

#include "../inc/logger_api.h"
#include "../error.h"


static int handle_pwd_change() {
    bool is_overflowed = false;
    char new_pwd[BUFF_PWD_LENGTH] = {0};

    get_path_to_root(new_pwd, BUFF_PWD_LENGTH, &is_overflowed);
    strncpy(buff_pwd, new_pwd, BUFF_PWD_LENGTH);

    if (is_overflowed) {
        puts("warning: pwd is too long");
    }

    return 0;
}


static int handle_prompt_change() {
    handle_pwd_change();

    snprintf(buff_prompt, BUFF_PROMPT_LENGTH, FORMAT_PROMPT, fs_name, buff_pwd);

    return 0;
}


int cd_(const char* path) {
    int ret = RETURN_FAILURE;
    // +2 if path == 0, then SEPARATOR with length 2 will be copied
    char target[strlen(path) + 2];
    struct inode in_cd;

    log_info("cd: [%s]", path);

    // target is either path, or root, when no path is given
    strlen(path) > 0 ? strcpy(target, path) : strcpy(target, SEPARATOR);

    if (get_inode_by_path(&in_cd, target) != RETURN_FAILURE) {
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
