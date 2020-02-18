#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/fs_cache.h"
#include "../inc/inode.h"
#include "../inc/return_codes.h"
#include "../fs_operations.h"

#include "../error.h"


int mkdir_(char* path) {
    int ret = RETURN_FAILURE;
    int32_t id = 0;

    // TODO
    //  - X! get destination inode
    //  - check last nonempty link in inode
    //  - check if in link's cluster is still space for another directory item
    //    - yes, mkdir -> get first free inode
    //    - no, use another link in inode[1], mkdir

    // TODO [1]
    //  - check if there is available link in inode (lvl1, lvl2, lvl3 link)
    //  - check if there is available data cluster
    //  -> create unit fs_operations.c

    if (strlen(path) > 0) {
        // get inode, where the new directory should be created in
        id = get_inodeid_by_path(path);

        if (id != RETURN_FAILURE) {
            // inode of last element in path gained
            FS_SEEK_SET(sb.addr_inodes + id);
            FS_READ(&in_distant, sizeof(struct inode), 1);

            // TODO continue here

            ret = RETURN_SUCCESS;
        }
    }
    else {
        set_myerrno(Arg_missing_operand);
    }

    return ret;
}
