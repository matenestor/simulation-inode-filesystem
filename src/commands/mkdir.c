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
    char* dir = NULL;
    struct inode source;;
    struct inode* p_source = NULL;

    if (strlen(path) > 0) {
        // track path from root
        if (path[0] == SEPARATOR[0]) {
            FS_SEEK_SET(sb.addr_inodes);
            FS_READ(&source, sizeof(struct inode), 1);
            p_source = &source;
        }
            // track path from actual directory
        else {
            p_source = &in_actual;
        }

        // parse path TODO
        for (dir = strtok(path, SEPARATOR); dir != NULL; dir = strtok(NULL, SEPARATOR)) {
            puts(dir);
        }

        // TODO
        //  - check last nonempty link in inode
        //  - check if in link's cluster is still space for another directory item
        //    - yes, mkdir -> get first free inode
        //    - no, use another link in inode[1], mkdir

        // TODO [1]
        //  - check if there is available link in inode (lvl1, lvl2, lvl3 link)
        //  - check if there is available data cluster
        //  -> create unit fs_operations.c

        ret = RETURN_SUCCESS;
    }
    else {
        set_myerrno(Arg_missing_operand);
    }

    return ret;
}
