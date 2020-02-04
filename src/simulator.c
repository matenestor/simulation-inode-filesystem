#include <stdio.h>
#include <string.h>

#include "inc/inode.h"
#include "inc/logger_api.h"
#include "inc/simulator.h"


void pr_usage() {
    printf(PR_USAGE);
}

void load(const char* fsn, const size_t fsname_len) {
    log_info("Loading filesystem with name [%s].", fsn);

    // TODO load if exists, notify about possible formatting

    strncpy(fsname, fsn, fsname_len);
    printf("%s\n", fsname);
}

void run() {
    // initial pwd is root
    strncpy(pwd, SEPARATOR, 1);
    snprintf(prompt, LENGTH_PROMPT_STRING, FORMAT_PROMPT, fsname, pwd);

    // TODO loop for accepting commands -- waiting? thread wait?

    printf("%s\n", prompt);

//    pr_usage();
}
