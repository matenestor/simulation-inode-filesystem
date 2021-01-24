#include <string.h>
#include <unistd.h>

#include "inode.h"

#include "../../include/logger.h"
#include "../../include/errors.h"
#include "../fs_operations.h"


int incp_(const char* source, const char* target) {
    int ret = RETURN_FAILURE;
    struct inode in_target = {0};

    if (strlen(source) > 0 && strlen(target) > 0) {
        // 'source' file exists in OS
        if (access(source, F_OK) == 0) {
            // inode, to copy file into, exists in filesystem
            if (get_inode_by_path(&in_target, target) != RETURN_FAILURE) {
                // todo fopen 'source' file and copy into inode
            }
            else if (create_inode(&in_target, Itemtype_file, 0 /* 0 here is wrong, should be id_parent */)) {
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

