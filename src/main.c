#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inc/error.h"
#include "inc/fspath_info.h"
#include "inc/logger_api.h"
#include "inc/main.h"
#include "inc/return_codes.h"


void signal_handler(int signum) {
    set_myerrno(Signal_interrupt);
    exit_error();
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
int parse_fsname(char* fsname, const char* arg_name) {
    // return value
    int rv = RETURN_SUCCESS;

    // length of provided name
    size_t len = strlen(arg_name);
    size_t i;

    if (len < STRLEN_FSNAME) {
        for (i = 0; i < len; ++i) {
            // not valid if char is not letter, number or underscore
            if (!(isalnum(arg_name[i]) || isunscr(arg_name[i]) || isdot(arg_name[i]))) {
                fprintf(stderr, "Use only letters, numbers, dots and/or underscores!\n");
                set_myerrno(Fsname_invalid);
                rv = RETURN_FAILURE;
            }
        }

        // parse name if is valid
        strncpy(fsname, arg_name, len);
    }
    else {
        fprintf(stderr, "Maximal length on name is 31 characters!\n");
        set_myerrno(Fsname_long);
        rv = RETURN_FAILURE;
    }

    return rv;
}

int main(int argc, char const **argv) {
    // name of filesystem given by user
    char fsname[STRLEN_FSNAME] = {0};

    // used for initialization
    reset_myerrno();

    // init logger and set level
    if (logger_init() == RETURN_SUCCESS) {
        logger_set_level(Debug);
    }

    // register signal interrupt
    signal(SIGINT, signal_handler);

    // if name was provided, try to parse it
    if (argc > 1) {
        parse_fsname(fsname, argv[1]);
    }
    // if no name was provided, set my_errno
    else {
        set_myerrno(Fsname_missing);
    }

    // if name is ok, load and run
    if (!is_error()) {
        printf(PR_INTRO);
        load(fsname);
        run();
    }
    // if error occurred during parsing process, exit with error
    else {
        printf(PR_HELP);
        exit_error();
    }

    // destroy logger
    logger_destroy();

    return EXIT_SUCCESS;
}
