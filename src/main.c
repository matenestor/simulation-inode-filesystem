#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "inc/fs_prompt.h"
#include "inc/return_codes.h"

#include "inc/logger_api.h"
#include "error.h"


void signal_handler(int signum) {
    close_filesystem();
    is_running = false;
    set_myerrno(Err_signal_interrupt);
    err_exit_msg();
    log_info("Exiting with error type: %s", my_strerror(my_errno));
    // with ctrl+C, program has to be terminated here, because else it waits for input
    // in simulator.c > handle_input(...) on fgets(...)
    // is there any way not to end it here?
    exit(EXIT_SUCCESS);
}


/******************************************************************************
 *
 * 	Copy filename, given as argument of program, to program buffer.
 *  Available are only letters, numbers, dots, underscores
 *  and slashes of total length of 'STRLEN_FSNAME'.
 *
 * 	On success, 0 is returned.
 * 	On error, -1 is returned, and my_errno is set appropriately.
 *
 */
int parse_fsname(char* fsn, const char* arg_name) {
    // return value
    int ret = RETURN_FAILURE;

    // length of provided name
    size_t len = strlen(arg_name);
    size_t i;

    if (len < STRLEN_FSNAME) {
        for (i = 0; i < len; ++i) {
            // not valid if char is not letter, number or underscore
            if (!(isalnum(arg_name[i]) || isunscr(arg_name[i]) || isdot(arg_name[i]) || isslash(arg_name[i]))) {
                fputs("Use only letters, numbers, dots and/or underscores!\n", stderr);
                set_myerrno(Err_fs_name_invalid);

                break;
            }
        }

        // parse name if is valid
        if (!is_error()) {
            strncpy(fsn, arg_name, len);

            ret = RETURN_SUCCESS;
        }
    }
    else {
        fprintf(stderr, "Maximal length on name is %d characters!\n", STRLEN_FSNAME);
        set_myerrno(Err_fs_name_long);
    }

    return ret;
}


int main(int argc, char const **argv) {
    #if DEBUG
    // Clion debugger output
    setbuf(stdout, 0);
    #endif

    int status_exit = RETURN_SUCCESS;

    // name of filesystem given by user
    char fsn[STRLEN_FSNAME] = {0};

    // init logger and set level
    if (logger_init() == RETURN_SUCCESS) {
        #if DEBUG
        logger_set_level(Log_Debug);
        #else
        logger_set_level(Log_Info);
        #endif
    }

    // used for initialization
    reset_myerrno();

    // register signal interrupt
    signal(SIGINT, signal_handler);

    // if name was provided, try to parse it
    if (argc > 1) {
        // if name is ok, run
        if (parse_fsname(fsn, argv[1]) == RETURN_SUCCESS) {
            puts(PR_INTRO);

            status_exit = init_simulation(fsn);
        }
        // if error occurred during parsing process, print error
        else {
            puts(PR_HELP);
            err_exit_msg();
            log_fatal("Exiting with error type: %s", my_strerror(my_errno));
        }
    }
    // if no name was provided, set my_errno
    else {
        puts(PR_HELP);
        set_myerrno(Err_fs_name_missing);
    }

    // destroy logger
    logger_destroy();

    return status_exit;
}
