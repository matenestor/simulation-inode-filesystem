#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

#define RETURN_SUCCESS 0
#define RETURN_FAILURE (-1)


enum error_ {
    Err_no_error			= 0,
    Err_signal_interrupt	= 1001,
    Err_fs_name_missing		= 1002,
    Err_fs_name_long		= 1003,
    Err_fs_size_sim_range	= 1004,
    Err_fs_size_sys_range	= 1005,
    Err_fs_size_nan			= 1006,
    Err_fs_size_none		= 1007,
    Err_fs_error			= 1008,
    Err_arg_missing			= 1009,
	Err_dir_full			= 1010,
    Err_dir_not_empty		= 1011,
    Err_dir_arg_invalid		= 1012,
    Err_item_exists			= 1013,
    Err_item_not_file		= 1014,
    Err_item_not_directory	= 1015,
    Err_item_not_exists		= 1016,
    Err_item_name_long		= 1017,
    Err_inode_no_links		= 1018,
    Err_inode_no_inodes		= 1019,
    Err_block_no_blocks		= 1020,
};

extern enum error_ my_errno;

bool is_error();
void reset_myerrno();
void set_myerrno(enum error_);
void my_perror(const char*);
const char* my_strerror(enum error_);

#endif
