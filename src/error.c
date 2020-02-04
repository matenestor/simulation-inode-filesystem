#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inc/error.h"
#include "inc/logger_api.h"


void reset_myerrno() {
    my_errno = No_error;
}

void set_myerrno(const error err) {
    my_errno = err;
}

char* my_strerror(const error err) {
    static char err_str[LENGTH_ERROR_STRING];

    switch (err) {
        case No_error:
            strcpy(err_str, "No error.");
            break;

        case Signal_interrupt:
            strcpy(err_str, "Signal interrupt.");
            break;

        case Fsname_missing:
            strcpy(err_str, "No filesystem name provided.");
            break;

        case Fsname_long:
            strcpy(err_str, "Filesystem name too long.");
            break;

        case Fsname_invalid:
            strcpy(err_str, "Filesystem name invalid.");
            break;

        default:
            strcpy(err_str, "");
    }

    return err_str;
}

void exit_error() {
    char* p_err_str = my_strerror(my_errno);

    // print error to console and log file
    fprintf(stderr, "%s", p_err_str);
    log_fatal("Exiting with error type: %s", p_err_str);

    // destroy logger
    logger_destroy();

    exit(EXIT_FAILURE);
}

bool is_error() {
    return my_errno != No_error;
}
