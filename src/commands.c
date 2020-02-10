#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inc/commands.h"
#include "inc/error.h"
#include "inc/inode.h"
#include "inc/logger_api.h"
#include "inc/return_codes.h"


int cp_(char* arg1, char* arg2) {
    int ret = RETURN_FAILURE;
    return ret;
}


int mv_(char* arg1, char* arg2) {
    int ret = RETURN_FAILURE;
    return ret;
}


int rm_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int mkdir_(char* path) {
    int ret = RETURN_FAILURE;
    char* dir = NULL;
    struct inode source;;
    struct inode* p_source = NULL;

    if (strlen(path) > 0) {
        // track path from root
        if (path[0] == '/') {
            fseek(filesystem, sb.addr_inodes, SEEK_SET);
            fread(&source, sizeof(struct inode), 1, filesystem);
            p_source = &source;
        }
        // track path from actual directory
        else {
            p_source = &in_actual;
        }

        // parse path TODO
        for (dir = strtok(path, "/"); dir != NULL; dir = strtok(NULL, "/")) {
            puts(dir);
        }

        // TODO
        //  - check last nonempty link in inode
        //  - check if in link's cluster is still space for another directory item
        //    - yes, mkdir
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


int rmdir_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int ls_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int cat_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int cd_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int pwd_() {
    int ret = RETURN_FAILURE;
    return ret;
}


int info_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int incp_(char* arg1, char* arg2) {
    int ret = RETURN_FAILURE;
    return ret;
}


int outcp_(char* arg1, char* arg2) {
    int ret = RETURN_FAILURE;
    return ret;
}


int load_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int fsck_() {
    int ret = RETURN_FAILURE;
    return ret;
}


int tree_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}
