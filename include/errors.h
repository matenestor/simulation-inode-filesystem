#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

#define RETURN_SUCCESS 0
#define RETURN_FAILURE -1


enum error_ {
    Err_no_error			= 0,
    Err_signal_interrupt	= 1001,
    Err_fs_name_missing		= 1002,
    Err_fs_name_long		= 1003,
    Err_fs_name_invalid		= 1004,
    Err_fs_not_loaded		= 1005,
    Err_fs_not_formatted	= 1006,
    Err_fs_size_sim_range	= 1007,
    Err_fs_size_sys_range	= 1008,
    Err_fs_size_nan			= 1009,
    Err_fs_size_negative	= 1010,
    Err_fs_size_none		= 1011,
    Err_fs_error			= 1012,
    Err_arg_missing			= 1013,
    Err_dir_not_empty		= 1014,
    Err_dir_arg_invalid		= 1015,
    Err_dir_exists			= 1016,
    Err_item_not_file		= 1017,
    Err_item_not_directory	= 1018,
    Err_item_not_exists		= 1019,
    Err_item_name_long		= 1020,
    Err_inode_no_links		= 1021,
    Err_inode_no_inodes		= 1022,
    Err_cluster_no_clusters	= 1023,
    Err_cluster_full		= 1024,
};

extern enum error_ my_errno;

bool is_error();
void reset_myerrno();
void set_myerrno(enum error_);
void my_perror(const char*);
const char* my_strerror(enum error_);

#endif
