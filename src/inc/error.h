#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

#define LENGTH_ERROR_STRING 64

enum error {
    No_error,
    Signal_interrupt,
    Fsname_missing,
    Fsname_long,
    Fsname_invalid,
    Fs_not_loaded,
    Fs_not_formatted,
};

enum error my_errno;

void reset_myerrno();
void set_myerrno(enum error);
char* my_strerror(enum error);
void my_perror();
void my_exit();
bool is_error();

#endif
