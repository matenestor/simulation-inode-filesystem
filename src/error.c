#include <stdio.h>
#include <string.h>

#include "error.h"


bool is_error() {
    return my_errno != Err_no_error;
}


void reset_myerrno() {
    my_errno = Err_no_error;
}


void set_myerrno(const enum __error err) {
    my_errno = err;
}


void my_perror(const char* msg) {
    fprintf(stderr, "%s: %s\n", msg, my_strerror(my_errno));
}


void err_exit_msg() {
    // print error to console and log file
    my_perror("exit");
}


char* my_strerror(const enum __error err) {
    static char err_str[__LENGTH_ERROR_STRING];

    switch (err) {
        case Err_no_error:
            strcpy(err_str, "no error");
            break;

        case Err_signal_interrupt:
            strcpy(err_str, "signal interrupt");
            break;

        case Err_fs_name_missing:
            strcpy(err_str, "filesystem name not provided");
            break;

        case Err_fs_name_long:
            strcpy(err_str, "filesystem name too long");
            break;

        case Err_fs_name_invalid:
            strcpy(err_str, "filesystem name invalid");
            break;

        case Err_fs_not_loaded:
            strcpy(err_str, "unable to load filesystem");
            break;

        case Err_fs_not_formatted:
            strcpy(err_str, "unable to format filesystem");
            break;

        case Err_fs_size_sim_range:
            strcpy(err_str, "filesystem size not in simulation range");
            break;

        case Err_fs_size_sys_range:
            strcpy(err_str, "filesystem size not in system range");
            break;

        case Err_fs_size_nan:
            strcpy(err_str, "filesystem size not a number");
            break;

        case Err_fs_size_negative:
            strcpy(err_str, "filesystem size negative");
            break;

        case Err_fs_size_none:
            strcpy(err_str, "filesystem size not provided");
            break;

        case Err_arg_missing:
            strcpy(err_str, "missing argument(s)");
            break;

        case Err_dir_not_empty:
            strcpy(err_str, "directory not empty");
            break;

        case Err_dir_arg_invalid:
            strcpy(err_str, "invalid argument");
            break;

        case Err_dir_exists:
            strcpy(err_str, "file exists");
            break;

        case Err_item_not_file:
            strcpy(err_str, "is a directory");
            break;

        case Err_item_not_directory:
            strcpy(err_str, "not a directory");
            break;

        case Err_item_not_exists:
            strcpy(err_str, "no such file or directory");
            break;

        case Err_item_name_long:
            strcpy(err_str, "item name too long");
            break;

        case Err_inode_no_inodes:
            strcpy(err_str, "no more inodes available");
            break;

        case Err_inode_no_links:
            strcpy(err_str, "no more links in inode available");
            break;

        case Err_cluster_no_clusters:
            strcpy(err_str, "no more data space available");
            break;

        case Err_cluster_full:
            strcpy(err_str, "cluster is full");
            break;

        case Err_fs_error:
            strcpy(err_str, "Filesystem internal error. Try to use command 'fsck'.");
            break;

        default:
            strcpy(err_str, "");
    }

    return err_str;
}
