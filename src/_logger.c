#include <sys/types.h>
#include <stdarg.h>
#include <time.h>

#include "_logger.h"
#include "inc/return_codes.h"


int logger_init() {
    int status = RETURN_SUCCESS;

    log_file = fopen(__LOG_FNAME, "w");

    if (log_file == NULL) {
        fputs("Log file could not be opened. Log messages will not be writen.\n", stderr);
        status = RETURN_FAILURE;
    }
    else {
        logger_set_level(Log_Info);
        log_info("Logger initialized.");
    }

    return status;
}


void logger_destroy() {
    if (log_file != NULL) {
        log_info("Closing logger.");
        fflush(log_file);
        fclose(log_file);
    }
}


void logger_set_level(enum __level lvl) {
    log_level = lvl;
}


void get_datetime(char* datetime) {
    static time_t t;
    static struct tm* tm_info;

    // get time
    t = time(NULL);
    tm_info = localtime(&t);

    strftime(datetime, __DATETIME_LENGTH, "[%Y-%m-%d %H:%M:%S]", tm_info);
}


void log_msg(const char* severity, const char* msg) {
    static char datetime[__DATETIME_LENGTH];
    get_datetime(datetime);

    // print log message to log file
    fprintf(log_file, "%s %s %s\n", severity, datetime, msg);
    fflush(log_file);
}


// ****************     LOG MESSAGES     **************************************


void log_fatal(const char* msg, ...) {
    if (log_level >= Log_Fatal && log_file != NULL) {
        // format msg in case of arguments
        va_list args;
        va_start(args, msg);
        vsnprintf(buff, __BUFF_SIZE, msg, args);
        va_end(args);

        // log message
        log_msg(__LOG_FATAL, buff);
    }
}


void log_error(const char* msg, ...) {
    if (log_level >= Log_Error && log_file != NULL) {
        // format msg in case of arguments
        va_list args;
        va_start(args, msg);
        vsnprintf(buff, __BUFF_SIZE, msg, args);
        va_end(args);

        // log message
        log_msg(__LOG_ERROR, buff);
    }
}


void log_warning(const char* msg, ...) {
    if (log_level >= Log_Warning && log_file != NULL) {
        // format msg in case of arguments
        va_list args;
        va_start(args, msg);
        vsnprintf(buff, __BUFF_SIZE, msg, args);
        va_end(args);

        // log message
        log_msg(__LOG_WARNING, buff);
    }
}


void log_info(const char* msg, ...) {
    if (log_level >= Log_Info && log_file != NULL) {
        // format msg in case of arguments
        va_list args;
        va_start(args, msg);
        vsnprintf(buff, __BUFF_SIZE, msg, args);
        va_end(args);

        // log message
        log_msg(__LOG_INFO, buff);
    }
}


void log_debug(const char* msg, ...) {
    if (log_level >= Log_Debug && log_file != NULL) {
        // format msg in case of arguments
        va_list args;
        va_start(args, msg);
        vsnprintf(buff, __BUFF_SIZE, msg, args);
        va_end(args);

        // log message
        log_msg(__LOG_DEBUG, buff);
    }
}


void log_trace(const char* msg, ...) {
    if (log_level == Log_Trace && log_file != NULL) {
        // format msg in case of arguments
        va_list args;
        va_start(args, msg);
        vsnprintf(buff, __BUFF_SIZE, msg, args);
        va_end(args);

        // log message
        log_msg(__LOG_TRACE, buff);
    }
}
