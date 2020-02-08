#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inc/error.h"
#include "inc/logger_api.h"


void reset_myerrno() {
    my_errno = No_error;
}


void set_myerrno(const enum error err) {
    my_errno = err;
}


char* my_strerror(const enum error err) {
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

        case Fs_not_loaded:
            strcpy(err_str, "Unable to load filesystem.");
            break;

        case Fs_not_formatted:
            strcpy(err_str, "Unable to format filesystem.");
            break;

        default:
            strcpy(err_str, "");
    }

    return err_str;
}


void my_perror() {
    fputs(my_strerror(my_errno), stderr);
    fputc('\n', stderr);
}


void my_exit() {
    // print error to console and log file
    my_perror();
    log_fatal("Exiting with error type: %s", my_strerror(my_errno));

    // destroy logger
    logger_destroy();

    exit(EXIT_FAILURE);
}


bool is_error() {
    return my_errno != No_error;
}
