#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "errors.h"


enum error_ my_errno;

bool is_error() {
	return my_errno != Err_no_error || errno != 0;
}

void reset_myerrno() {
	errno = 0;
	my_errno = Err_no_error;
}

void set_myerrno(const enum error_ err) {
	my_errno = err;
}

void my_perror(const char* msg) {
	if (my_errno != Err_no_error) {
		fprintf(stderr, "%s: %s\n", msg, my_strerror(my_errno));
	} else if (errno != 0) {
		perror(msg);
	}
}

const char* my_strerror(const enum error_ err) {
	switch (err) {
		case Err_no_error:				return "no error";
		case Err_signal_interrupt:		return "signal interrupt";
		case Err_fs_name_missing:		return "filesystem name not provided";
		case Err_fs_name_long:			return "filesystem name too long";
		case Err_fs_not_loaded:			return "unable to load filesystem";
		case Err_fs_size_sim_range:		return "filesystem size not in simulation range";
		case Err_fs_size_sys_range:		return "filesystem size not in system range";
		case Err_fs_size_nan:			return "filesystem size not a number";
		case Err_fs_size_none:			return "filesystem size not provided";
		case Err_arg_missing:			return "missing operand";
		case Err_dir_not_empty:			return "directory not empty";
		case Err_dir_arg_invalid:		return "invalid argument";
		case Err_item_exists:			return "file exists";
		case Err_item_not_file:			return "is a directory";
		case Err_item_not_directory:	return "not a directory";
		case Err_item_not_exists:		return "no such file or directory";
		case Err_item_name_long:		return "item name too long";
		case Err_inode_no_inodes:		return "no more inodes available";
		case Err_inode_no_links:		return "no more links in inode available";
		case Err_block_no_blocks:		return "no more data space available";
		case Err_block_full:			return "block is full";
		case Err_fs_error:				return "Filesystem internal error. Try to use command 'fsck'.";
		default:						return strerror(errno);
	}
}
