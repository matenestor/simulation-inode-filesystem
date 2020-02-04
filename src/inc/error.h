#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

#define LENGTH_ERROR_STRING 64

typedef enum {
    No_error,
    Signal_interrupt,
    Fsname_missing,
    Fsname_long,
    Fsname_invalid,
} error;

error my_errno;

void reset_myerrno();
void set_myerrno(error);
char* my_strerror(error);
void exit_error();
bool is_error();

#endif
