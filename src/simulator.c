#include <stdio.h>
#include <string.h>

#include "simulator.h"
#include "inc/return_codes.h"
#include "utils.h"

#include "inc/logger_api.h"
#include "error.h"


static void init_prompt() {
    // create initial pwd, 2 for null terminator also
    strncpy(buff_pwd, SEPARATOR, 2);
    // create prompt
    snprintf(buff_prompt, BUFF_PROMPT_LENGTH, FORMAT_PROMPT, fs_name, buff_pwd);
}


/******************************************************************************
 *
 *  If EOF was not sent to console and input is in buffer range,
 *  parse command and its arguments, if any.
 *
 *  On success, 0 is returned. On error, -1 is returned.
 *
 */
static int handle_input(char* command, char* arg1, char* arg2) {
    int ret = RETURN_FAILURE;

    // discard char
    int c = 0;
    // program buffer for user's inputs; 3x more for 'command', 'arg1' and 'arg2'
    static char buff_in[3 * BUFF_IN_LENGTH] = {0};

    if ((fgets(buff_in, 3 * BUFF_IN_LENGTH, stdin) != NULL)) {
        // prevent overflowing input, so it doesn't go to next cycle
        if (isoverflow(buff_in[3 * BUFF_IN_LENGTH - 2])) {
            // discard everything in stdin and move on
            while ((c = fgetc(stdin)) != '\n' && c != EOF);
            puts("input too large");
        }
        else {
            // input from user is ok, so scan it
            if (sscanf(buff_in, "%s %s %s", command, arg1, arg2) > 0) {
                ret = RETURN_SUCCESS;
            }
        }
    }
    else {
        // EOF sent to console, print new line
        puts("");
    }

    return ret;
}


/******************************************************************************
 *
 *  In simulation cycle handles input from user, if in correct form,
 *  function checks if given command is known and calls it.
 *  If filesystem was not formatted yet, user is allowed to use only
 *  'format', 'help' and 'exit' commands. After formatting, user may use
 *  all other commands.
 *
 */
static void run() {
    char command[BUFF_IN_LENGTH] = {0};
    char arg1[BUFF_IN_LENGTH] = {0};
    char arg2[BUFF_IN_LENGTH] = {0};

    while (is_running) {
        // print prompt
        fputs(buff_prompt, stdout);

        // parse input and check which command it is
        if (handle_input(command, arg1, arg2) == RETURN_SUCCESS) {

            // allowed commands without formatting
            if (strcmp(command, CMD_FORMAT) == 0) {
                if (format_(arg1, fs_name) == RETURN_FAILURE) {
                    my_perror(CMD_FORMAT);
                    reset_myerrno();
                    is_formatted = false;
                }
                else {
                    init_prompt();
                    is_formatted = true;
                }
            }

            else if (strcmp(command, CMD_HELP) == 0) {
                puts(PR_USAGE);
            }

            else if (strcmp(command, CMD_EXIT) == 0) {
                close_filesystem();
                is_running = false;
            }

            // after filesystem is formatted, all commands are allowed
            else if (is_formatted) {
                if (strcmp(command, CMD_MKDIR) == 0) {
                    if (mkdir_(arg1) == RETURN_FAILURE) {
                        my_perror(CMD_MKDIR);
                        reset_myerrno();
                    }
                }

                else if (strcmp(command, CMD_LS) == 0) {
                    if (ls_(arg1) == RETURN_FAILURE) {
                        my_perror(CMD_LS);
                        reset_myerrno();
                    }
                }

                else if (strcmp(command, CMD_PWD) == 0) {
                    pwd_();
                }

                else if (strcmp(command, CMD_CD) == 0) {
                    if (cd_(arg1) == RETURN_FAILURE) {
                        my_perror(CMD_CD);
                        reset_myerrno();
                    }
                }

                else if (strcmp(command, CMD_INFO) == 0) {
                    if (info_(arg1) == RETURN_FAILURE) {
                        my_perror(CMD_INFO);
                        reset_myerrno();
                    }
                }

                else if (strcmp(command, CMD_RMDIR) == 0) {
                    if (rmdir_(arg1) == RETURN_FAILURE) {
                        my_perror(CMD_RM);
                        reset_myerrno();
                    }
                }

                else if (strcmp(command, CMD_INCP) == 0) {
                    if (incp_(arg1, arg2) == RETURN_FAILURE) {
                        my_perror(CMD_INCP);
                        reset_myerrno();
                    }
                }

                else if (strcmp(command, CMD_OUTCP) == 0) {
                    if (outcp_(arg1, arg2) == RETURN_FAILURE) {
                        my_perror(CMD_OUTCP);
                        reset_myerrno();
                    }
                }

                else if (strcmp(command, CMD_CP) == 0) {
                    cp_(arg1, arg2);
                }

                else if (strcmp(command, CMD_MV) == 0) {
                    mv_(arg1, arg2);
                }

                else if (strcmp(command, CMD_RM) == 0) {
                    rm_(arg1);
                }

                else if (strcmp(command, CMD_CAT) == 0) {
                    cat_(arg1);
                }
                else if (strcmp(command, CMD_LOAD) == 0) {
                    load_(arg1);
                }

                else if (strcmp(command, CMD_FSCK) == 0) {
                    fsck_();
                }

                else if (strcmp(command, CMD_TREE) == 0) {
                    tree_(arg1);
                }

                else if (strcmp(command, CMD_DEBUG) == 0) {
                    debug_(arg1);
                }

                else {
                    puts("-zos: command not found");
                }
            }
            else {
                puts("Filesystem not formatted. You can format one with command 'format <size>'.");
            }
        }

        // input buffer is small, so just clear it all
        BUFF_CLR(command, strlen(command));
        // buffers for arguments are cleared only as much as necessary (a bit of optimization)
        BUFF_CLR(arg1, strlen(arg1));
        BUFF_CLR(arg2, strlen(arg2));
    }
}


/******************************************************************************
 *
 *  If simulation is successfully initialized, then function runs it.
 *  Else terminates with error.
 *
 */
int init_simulation(const char* fsp) {
    int status_simulation;

    if (parse_name(fs_name, fsp, STRLEN_FSNAME) == RETURN_SUCCESS) {
        init_prompt();

        // if filesystem exists and was loaded successfully, or user was notified about
        // possible formatting, prepare for simulation
        if (init_filesystem(fsp, &is_formatted) == RETURN_SUCCESS) {
            status_simulation = RETURN_SUCCESS;
            is_running = true;

            puts("Type 'help' for more information.");

            // run simulation
            run();
        }
        else {
            status_simulation = RETURN_FAILURE;
            is_running = false;

            err_exit_msg();
        }
    }
    else {
        is_running = false;
        status_simulation = RETURN_FAILURE;
        fprintf(stderr, "Maximal length of filesystem name is %d characters!\n", STRLEN_FSNAME);
    }

    log_info("Simulation end.");

    return status_simulation;
}








int cp_(const char* arg1, const char* arg2) {
    int ret = RETURN_FAILURE;
    return ret;
}


int mv_(const char* arg1, const char* arg2) {
    int ret = RETURN_FAILURE;
    return ret;
}


int rm_(const char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int cat_(const char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int load_(const char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int fsck_() {
    int ret = RETURN_FAILURE;
    return ret;
}


int tree_(const char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}
