#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../inc/return_codes.h"
#include "../inc/logger_api.h"
#include "../error.h"

int rmdir_(const char* path) {
    int ret = RETURN_FAILURE;

    if (strlen(path) > 0) {

    }
    else {
        set_myerrno(Err_arg_missing_operand);
        log_warning("rmdir: unable to remove directory [%s]", path);
    }

    return ret;
}
