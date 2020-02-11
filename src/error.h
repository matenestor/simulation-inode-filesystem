#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

#define __LENGTH_ERROR_STRING 64

enum __error {
    No_error,
    Signal_interrupt,
    Fs_name_missing,
    Fs_name_long,
    Fs_name_invalid,
    Fs_not_loaded,
    Fs_not_formatted,
    Fs_size_sim_range,
    Fs_size_sys_range,
    Fs_size_nan,
    Fs_size_negative,
    Fs_size_none,
    Arg_missing_operand,
    Arg_missing_destination,
    Dir_not_empty,
    Dir_exists,
    Item_not_file,
    Item_not_directory,
    Item_not_exists,
};

enum __error my_errno;

void reset_myerrno();
void set_myerrno(enum __error);
char* my_strerror(enum __error);
void my_perror(const char*);
void my_exit();
bool is_error();

#endif