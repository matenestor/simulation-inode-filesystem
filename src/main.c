#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "inc/fs_path.h"
#include "inc/return_codes.h"

#include "inc/logger_api.h"
#include "error.h"


void signal_handler(int signum) {
    close_filesystem();
    set_myerrno(Err_signal_interrupt);
    my_exit();
}


/******************************************************************************
 *
 * 	Copy filename, given as argument of program, to program buffer.
 *  Available are only letters, numbers, dots and underscores of total length of 31.
 *
 * 	On success, 0 is returned.
 * 	On error, -1 is returned, and my_errno is set appropriately.
 *
 */
int parse_fsname(char* fs_name, const char* arg_name) {
    // return value
    int ret = RETURN_SUCCESS;

    // length of provided name
    size_t len = strlen(arg_name);
    size_t i;

    if (len < STRLEN_FSNAME) {
        for (i = 0; i < len; ++i) {
            // not valid if char is not letter, number or underscore
            if (!(isalnum(arg_name[i]) || isunscr(arg_name[i]) || isdot(arg_name[i]))) {
                fputs("Use only letters, numbers, dots and/or underscores!\n", stderr);
                set_myerrno(Err_fs_name_invalid);
                ret = RETURN_FAILURE;
            }
        }

        // parse name if is valid
        strncpy(fs_name, arg_name, len);
    }
    else {
        fputs("Maximal length on name is 31 characters!\n", stderr);
        set_myerrno(Err_fs_name_long);
        ret = RETURN_FAILURE;
    }

    return ret;
}


int main(int argc, char const **argv) {
    // name of filesystem given by user
    char fs_name[STRLEN_FSNAME] = {0};

    // used for initialization
    reset_myerrno();

    // init logger and set level
    if (logger_init() == RETURN_SUCCESS) {
        logger_set_level(Log_Debug);
    }

    // register signal interrupt
    signal(SIGINT, signal_handler);

    // if name was provided, try to parse it
    if (argc > 1) {
        parse_fsname(fs_name, argv[1]);
    }
    // if no name was provided, set my_errno
    else {
        set_myerrno(Err_fs_name_missing);
    }

    // if name is ok, load and run
    if (!is_error()) {
        puts(PR_INTRO);

        // run, if filesystem was loaded successfully, when exists,
        // or user was notified about possible formatting, when doesn't exists
        if (load(fs_name) == RETURN_SUCCESS) {
            run();
        }
        else {
            my_exit();
        }
    }
    // if error occurred during parsing process, exit with error
    else {
        puts(PR_HELP);
        my_exit();
    }

    // destroy logger
    logger_destroy();

    return EXIT_SUCCESS;
}
