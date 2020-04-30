#include <stdio.h>
#include <string.h>

#include "simulator.h"
#include "inc/return_codes.h"

#include "inc/logger_api.h"
#include "error.h"


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
    static char buff_in[3 * BUFFIN_LENGTH] = {0};

    if ((fgets(buff_in, 3 * BUFFIN_LENGTH, stdin) != NULL)) {
        // prevent overflowing input, so it doesn't go to next cycle
        if (isoverflow(buff_in[3 * BUFFIN_LENGTH - 2])) {
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


static void run() {
    char command[BUFFIN_LENGTH] = {0};
    char arg1[BUFFIN_LENGTH] = {0};
    char arg2[BUFFIN_LENGTH] = {0};

    while (is_running) {
        // print prompt
        fputs(buff_prompt, stdout);

        // parse input and check which command it is
        if (handle_input(command, arg1, arg2) == RETURN_SUCCESS) {

            // allowed commands without formatting
            if (strcmp(command, CMD_FORMAT) == 0) {
                if (format_(arg1, fs_name) == RETURN_FAILURE) {
                    my_perror("format");
                    reset_myerrno();
                    log_error("Filesystem could not be formatted.");
                }
                else {
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
                if (strcmp(command, CMD_CP) == 0) {
                    cp_(arg1, arg2);
                }

                else if (strcmp(command, CMD_MV) == 0) {
                    mv_(arg1, arg2);
                }

                else if (strcmp(command, CMD_RM) == 0) {
                    rm_(arg1);
                }

                else if (strcmp(command, CMD_MKDIR) == 0) {
                    if (mkdir_(arg1) == RETURN_FAILURE) {
                        my_perror("mkdir");
                        reset_myerrno();
                    }
                }

                else if (strcmp(command, CMD_RMDIR) == 0) {
                    rmdir_(arg1);
                }

                else if (strcmp(command, CMD_LS) == 0) {
                    if (ls_(arg1) == RETURN_FAILURE) {
                        my_perror("ls");
                        reset_myerrno();
                    }
                }

                else if (strcmp(command, CMD_CAT) == 0) {
                    cat_(arg1);
                }

                else if (strcmp(command, CMD_CD) == 0) {
                    cd_(arg1);
                }

                else if (strcmp(command, CMD_PWD) == 0) {
                    pwd_();
                }

                else if (strcmp(command, CMD_INFO) == 0) {
                    info_(arg1);
                }

                else if (strcmp(command, CMD_INCP) == 0) {
                    incp_(arg1, arg2);
                }

                else if (strcmp(command, CMD_OUTCP) == 0) {
                    outcp_(arg1, arg2);
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


int init_simulation(const char* fsn) {
    int status_simulation;

    // cache filesystem name
    strncpy(fs_name, fsn, strlen(fsn));

    // create prompt, initial pwd is root
    snprintf(buff_prompt, STRLEN_PROMPT, FORMAT_PROMPT, fs_name, SEPARATOR);

    // if filesystem exists and was loaded successfully, or user was notified about
    // possible formatting, prepare for simulation
    if (init_filesystem(fs_name, &is_formatted) == RETURN_SUCCESS) {
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


int rmdir_(const char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int cat_(const char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int cd_(const char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int info_(const char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int incp_(const char* arg1, const char* arg2) {
    int ret = RETURN_FAILURE;
    return ret;
}


int outcp_(const char* arg1, const char* arg2) {
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
