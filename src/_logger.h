#ifndef _LOGGER_H
#define _LOGGER_H

#include <stdio.h>

#include "logger_api.h"


/** Size of buffer for logging. */
#define __BUFF_SIZE 1024

/** Length of datetime string + \0 */
#define __DATETIME_LENGTH 22

/** Log file name. */
#define __LOG_FNAME   "log/inode.log"

// severity message constants
#define __LOG_FATAL   "[FATAL]  "
#define __LOG_ERROR   "[ERROR]  "
#define __LOG_WARNING "[WARNING]"
#define __LOG_INFO    "[INFO]   "
#define __LOG_DEBUG   "[DEBUG]  "
#define __LOG_TRACE   "[TRACE]  "


/** Buffering messages for logging. */
static char buff[__BUFF_SIZE];

/** Level of logger severity. */
static enum __level log_level;

/** File for logging to. */
static FILE* log_file;

/** Get date and time in format %d.%m.%y %H:%M:%S */
static void get_datetime(char*);

/** Prints final message to file. */
static void log_msg(const char*, const char*);


#endif
