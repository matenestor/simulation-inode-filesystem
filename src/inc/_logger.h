#ifndef _LOGGER_H
#define _LOGGER_H

#include <stdio.h>

#include "logger_api.h"


/** Size of buffer for logging. */
#define BUFF_SIZE 1024

/** Length of datetime string + \0 */
#define DATETIME_LENGTH 21

/** Log file name. */
#define LOG_FNAME   "log/inode.log"

// severity message constants
#define LOG_FATAL   "[FATAL]  "
#define LOG_ERROR   "[ERROR]  "
#define LOG_WARNING "[WARNING]"
#define LOG_INFO    "[INFO]   "
#define LOG_DEBUG   "[DEBUG]  "
#define LOG_TRACE   "[TRACE]  "


/** Buffering messages for logging. */
static char buff[BUFF_SIZE];

/** Level of logger severity. */
static enum level log_level;

/** File for logging to. */
static FILE* log_file;

/** Get date and time in format %d.%m.%y %H:%M:%S */
void get_datetime(char*);

/** Prints final message to file. */
static void log_msg(const char*, const char*);


#endif
