#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "inc/error.h"
#include "inc/logger_api.h"
#include "inc/return_codes.h"
#include "inc/simulator.h"


void clear_buffer(char* buff, const size_t size) {
    memset(buff, '\0', size);
}

int load(const char* fsn) {
    int ret = RETURN_SUCCESS;

    log_info("Loading filesystem with name [%s].", fsn);

    // cache filesystem name (only fs name)
    strncpy(fsname, fsn, STRLEN_FSNAME);
    // cache filesystem path (both fs dir and fs name)
    snprintf(fspath, STRLEN_FSPATH, FORMAT_FSDIR, fsname);

    // create prompt, initial pwd is root
    snprintf(buff_prompt, STRLEN_PROMPT, FORMAT_PROMPT, fsname, SEPARATOR);

    // if filesystem exists, load it
    if (access(fspath, F_OK) == RETURN_SUCCESS) {
        // filesystem is ready to be loaded
        if ((filesystem = fopen(fspath, "rb+")) != NULL) {
            puts("Filesystem loaded successfully.");
            puts(PR_TRY_HELP);

            log_info("Filesystem [%s] loaded.", fspath);
        }
        // filesystem is ready to be loaded, but there was an error
        else {
            set_myerrno(Fs_not_loaded);
            ret = RETURN_FAILURE;
        }
    }
    // else notify about possible formatting
    else {
        puts("No filesystem with this name found. You can format one with command 'format <size>'.");
        puts(PR_TRY_HELP);

        log_info("Filesystem [%s] not found.", fspath);
    }

    return ret;
}

/******************************************************************************
 *
 *  If EOF was not sent to console and input is in buffer range,
 *  parse command and its arguments, if any.
 *
 *  On success, 0 is returned. On error, -1 is returned.
 *
 */
int handle_input(char* command, char* arg1, char* arg2) {
    int ret = RETURN_FAILURE;

    // discard char
    int c = 0;
    // program buffer for user's inputs
    static char buff_in[BUFFIN_LENGTH] = {0};

    if ((fgets(buff_in, BUFFIN_LENGTH, stdin) != NULL)) {
        // prevent overflowing input, so it doesn't go to next cycle
        if (isoverflow(buff_in[BUFFIN_LENGTH - 2])) {
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


void run() {
    // space only for longest command is enough
    char command[STRLEN_LONGEST_CMD] = {0};
    // arguments for paths can be as long as whole input buffer (without some chars, but just leave it)
    char arg1[BUFFIN_LENGTH] = {0};
    char arg2[BUFFIN_LENGTH] = {0};

    bool is_running = true;

    while (is_running) {
        // print prompt
        fputs(buff_prompt, stdout);

        // parse input and check which command it is
        if (handle_input(command, arg1, arg2) == RETURN_SUCCESS) {

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
                mkdir_(arg1);
            }

            else if (strcmp(command, CMD_RMDIR) == 0) {
                rmdir_(arg1);
            }

            else if (strcmp(command, CMD_LS) == 0) {
                ls_(arg1);
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

            else if (strcmp(command, CMD_FORMAT) == 0) {
                if (format_(arg1, filesystem, fspath) == RETURN_FAILURE) {
                    my_perror();
                    reset_myerrno();
                    perror("sys error");
                }
            }

            else if (strcmp(command, CMD_FSCK) == 0) {
                fsck_();
            }

            else if (strcmp(command, CMD_TREE) == 0) {
                tree_(arg1);
            }

            else if (strcmp(command, CMD_HELP) == 0) {
                puts(PR_USAGE);
            }

            else if (strcmp(command, CMD_EXIT) == 0) {
                if (filesystem != NULL) {
                    fclose(filesystem);
                }

                is_running = false;
            }

            else {
                puts("command not found");
                puts(PR_TRY_HELP);
            }
        }

        // input buffer is small, so just clear it all
        clear_buffer(command, BUFFIN_LENGTH);
        // buffers for arguments are cleared only as much as necessary (a bit of optimization)
        clear_buffer(arg1, strlen(arg1));
        clear_buffer(arg2, strlen(arg2));
    }

    log_info("Simulation end.");
}
