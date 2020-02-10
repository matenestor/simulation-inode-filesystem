#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inc/error.h"
#include "inc/logger_api.h"


void reset_myerrno() {
    my_errno = No_error;
}


void set_myerrno(const enum __error err) {
    my_errno = err;
}


char* my_strerror(const enum __error err) {
    static char err_str[__LENGTH_ERROR_STRING];

    switch (err) {
        case No_error:
            strcpy(err_str, "no error");
            break;

        case Signal_interrupt:
            strcpy(err_str, "signal interrupt");
            break;

        case Fs_name_missing:
            strcpy(err_str, "filesystem name not provided");
            break;

        case Fs_name_long:
            strcpy(err_str, "filesystem name too long");
            break;

        case Fs_name_invalid:
            strcpy(err_str, "filesystem name invalid");
            break;

        case Fs_not_loaded:
            strcpy(err_str, "unable to load filesystem");
            break;

        case Fs_not_formatted:
            strcpy(err_str, "unable to format filesystem");
            break;

        case Fs_size_sim_range:
            strcpy(err_str, "filesystem size not in simulation range");
            break;

        case Fs_size_sys_range:
            strcpy(err_str, "filesystem size not in system range");
            break;

        case Fs_size_nan:
            strcpy(err_str, "filesystem size not a number");
            break;

        case Fs_size_negative:
            strcpy(err_str, "filesystem size negative");
            break;

        case Fs_size_none:
            strcpy(err_str, "filesystem size not provided");
            break;

        case Arg_missing_operand:
            strcpy(err_str, "missing operand");
            break;

        case Arg_missing_destination:
            strcpy(err_str, "missing destination file operand");
            break;

        case Dir_not_empty:
            strcpy(err_str, "directory not empty");
            break;

        case Dir_exists:
            strcpy(err_str, "file exists");
            break;

        case Item_not_file:
            strcpy(err_str, "is a directory");
            break;

        case Item_not_directory:
            strcpy(err_str, "not a directory");
            break;

        case Item_not_exists:
            strcpy(err_str, "no such file or directory");
            break;

        default:
            strcpy(err_str, "");
    }

    return err_str;
}


void my_perror(const char* msg) {
    fprintf(stderr, "%s: %s\n", msg, my_strerror(my_errno));
}


void my_exit() {
    // print error to console and log file
    my_perror("exit");
    log_fatal("Exiting with error type: %s", my_strerror(my_errno));

    // destroy logger
    logger_destroy();

    exit(EXIT_FAILURE);
}


bool is_error() {
    return my_errno != No_error;
}
