#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "simulator.h"
#include "inc/return_codes.h"

#include "inc/logger_api.h"
#include "error.h"


void close_filesystem() {
    if (FS_VARIABLE_NAME != NULL) {
        fclose(FS_VARIABLE_NAME);
        log_info("Filesystem [%s] closed.", fs_name);
    }
}


int load(const char* fsn) {
    int ret = RETURN_SUCCESS;

    log_info("Loading filesystem with name [%s].", fsn);

    // cache filesystem name (only fs name)
    strncpy(fs_name, fsn, STRLEN_FSNAME);
    // cache filesystem path (both fs dir and fs name)
    snprintf(fs_path, STRLEN_FSPATH, FORMAT_FSDIR, fs_name);

    // create prompt, initial pwd is root
    snprintf(buff_prompt, STRLEN_PROMPT, FORMAT_PROMPT, fs_name, SEPARATOR);

    // if filesystem exists, load it
    if (access(fs_path, F_OK) == RETURN_SUCCESS) {
        // filesystem is ready to be loaded
        if ((FS_VARIABLE_NAME = fopen(fs_path, "rb+")) != NULL) {
            // cache super block
            FS_READ(&sb, sizeof(struct superblock), 1);
            // move to inodes location
            FS_SEEK_SET(sb.addr_inodes);
            // cache root inode
            FS_READ(&in_actual, sizeof(struct inode), 1);

            puts("Filesystem loaded successfully.");
            puts(PR_TRY_HELP);

            log_info("Filesystem [%s] loaded.", fs_path);
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

        log_info("Filesystem [%s] not found.", fs_path);
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

            // TODO do not allow commands, except for 'format, help, exit', when filesystem is not formatted

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
                if (format_(arg1, fs_path) == RETURN_FAILURE) {
                    my_perror("format");
                    reset_myerrno();
                    log_error("Filesystem could not be formatted.");
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
                close_filesystem();
                is_running = false;
            }

            else {
                puts("zos: command not found");
                puts(PR_TRY_HELP);
            }
        }

        // input buffer is small, so just clear it all
        BUFF_CLR(command, STRLEN_LONGEST_CMD);
        // buffers for arguments are cleared only as much as necessary (a bit of optimization)
        BUFF_CLR(arg1, strlen(arg1));
        BUFF_CLR(arg2, strlen(arg2));
    }

    log_info("Simulation end.");
}









int cp_(char* arg1, char* arg2) {
    int ret = RETURN_FAILURE;
    return ret;
}


int mv_(char* arg1, char* arg2) {
    int ret = RETURN_FAILURE;
    return ret;
}


int rm_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int rmdir_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int ls_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int cat_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int cd_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int pwd_() {
    int ret = RETURN_FAILURE;
    return ret;
}


int info_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int incp_(char* arg1, char* arg2) {
    int ret = RETURN_FAILURE;
    return ret;
}


int outcp_(char* arg1, char* arg2) {
    int ret = RETURN_FAILURE;
    return ret;
}


int load_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}


int fsck_() {
    int ret = RETURN_FAILURE;
    return ret;
}


int tree_(char* arg1) {
    int ret = RETURN_FAILURE;
    return ret;
}
