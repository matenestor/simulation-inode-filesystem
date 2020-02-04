#include <stdio.h>
#include <string.h>
#include "unistd.h"

#include "inc/error.h"
#include "inc/inode.h"
#include "inc/logger_api.h"
#include "inc/return_codes.h"
#include "inc/simulator.h"


void pr_usage() {
    printf(PR_USAGE);
}

void load(const char* fsn) {
    log_info("Loading filesystem with name [%s].", fsn);

    // cache filesystem name (only fs name)
    strncpy(fsname, fsn, STRLEN_FSNAME);
    // cache filesystem path (both fs dir and fs name)
    snprintf(fspath, STRLEN_FSPATH, FORMAT_FSDIR, fsname);

    // initial pwd is root
    strncpy(pwd, SEPARATOR, 1);
    // create prompt
    snprintf(prompt, STRLEN_PROMPT, FORMAT_PROMPT, fsname, pwd);

    // TODO load if exists, notify about possible formatting
    if (access(fspath, F_OK) == RETURN_SUCCESS) {
        printf("exists\n");

        FILE* filesystem = fopen(fspath, "wb+");

        if (filesystem != NULL) {
            printf("ok\n");
        }
        else {
            printf("error\n");
        }
    }
    else {
        printf("nope\n");

        fopen(fspath, "a");
    }
}

void run() {
    // TODO loop for accepting commands -- waiting? thread wait?

    printf("%s\n", prompt);

//    pr_usage();
}
