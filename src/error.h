#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

#define __LENGTH_ERROR_STRING 64

enum __error {
    Err_no_error,
    Err_signal_interrupt,
    Err_fs_name_missing,
    Err_fs_name_long,
    Err_fs_name_invalid,
    Err_fs_not_loaded,
    Err_fs_not_formatted,
    Err_fs_size_sim_range,
    Err_fs_size_sys_range,
    Err_fs_size_nan,
    Err_fs_size_negative,
    Err_fs_size_none,
    Err_fs_error,
    Err_arg_missing_operand,
    Err_arg_missing_destination,
    Err_dir_not_empty,
    Err_dir_exists,
    Err_item_not_file,
    Err_item_not_directory,
    Err_item_not_exists,
    Err_item_name_long,
    Err_inode_no_links,
    Err_inode_no_inodes,
    Err_cluster_no_clusters,
    Err_cluster_full,
};

enum __error my_errno;

void reset_myerrno();
void set_myerrno(enum __error);
char* my_strerror(enum __error);
void my_perror(const char*);
void my_exit();
bool is_error();

#endif
