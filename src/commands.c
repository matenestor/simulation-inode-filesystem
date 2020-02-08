#include <stdio.h>

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


int mkdir_(char* arg1) {
    int ret = RETURN_FAILURE;
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


int format_(const char* arg1, FILE* filesystem, const char* path) {
    int ret = RETURN_FAILURE;

    // TODO convert arg1 to int, if any
    // TODO write structures to file

    // filesystem is ready to be loaded
    if ((filesystem = fopen(path, "rb+")) != NULL) {
        fwrite("open", sizeof(char), 4, filesystem);
        puts("Filesystem formatted.");
        ret = RETURN_SUCCESS;
    }
    else {
        set_myerrno(Fs_not_formatted);
    }

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
