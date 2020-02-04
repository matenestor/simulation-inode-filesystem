#include <sys/types.h>
#include <stdarg.h>
#include <time.h>

#include "inc/_logger.h"
#include "inc/return_codes.h"


int logger_init() {
    int status = RETURN_SUCCESS;

    log_file = fopen(LOG_FNAME, "w");

    if (log_file == NULL) {
        printf("[WARNING] Log file could not be opened. Log messages will not be writen.");
        status = RETURN_FAILURE;
    }
    else {
        logger_set_level(Info);
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


void logger_set_level(level lvl) {
    log_level = lvl;
}


void get_datetime(char* datetime) {
    static time_t t;
    static struct tm* tm_info;

    // get time
    t = time(NULL);
    tm_info = localtime(&t);

    // year-month-day_hours-minutes-seconds
    strftime(datetime, DATETIME_LENGTH, "[%d.%m.%y %H:%M:%S] ", tm_info);
}


void log_msg(const char* severity, const char* msg) {
    static char datetime[DATETIME_LENGTH];
    get_datetime(datetime);

    // no need to create another buffer and use snprintf,
    // put it directly to file stream, instead of ram and then file stream
    fputs(severity, log_file);
    fputs(datetime, log_file);
    fputs(msg, log_file);
    fputc('\n', log_file);
    fflush(log_file);
}


// ****************     LOG MESSAGES     **************************************


void log_fatal(const char* msg, ...) {
    if (log_level >= Fatal && log_file != NULL) {
        // format msg in case of arguments
        va_list args;
        va_start(args, msg);
        vsnprintf(buff, BUFF_SIZE, msg, args);
        va_end(args);

        // log message
        log_msg(LOG_FATAL, buff);
    }
}

void log_error(const char* msg, ...) {
    if (log_level >= Error && log_file != NULL) {
        // format msg in case of arguments
        va_list args;
        va_start(args, msg);
        vsnprintf(buff, BUFF_SIZE, msg, args);
        va_end(args);

        // log message
        log_msg(LOG_ERROR, buff);
    }
}

void log_warning(const char* msg, ...) {
    if (log_level >= Warning && log_file != NULL) {
        // format msg in case of arguments
        va_list args;
        va_start(args, msg);
        vsnprintf(buff, BUFF_SIZE, msg, args);
        va_end(args);

        // log message
        log_msg(LOG_WARNING, buff);
    }
}

void log_info(const char* msg, ...) {
    if (log_level >= Info && log_file != NULL) {
        // format msg in case of arguments
        va_list args;
        va_start(args, msg);
        vsnprintf(buff, BUFF_SIZE, msg, args);
        va_end(args);

        // log message
        log_msg(LOG_INFO, buff);
    }
}

void log_debug(const char* msg, ...) {
    if (log_level >= Debug && log_file != NULL) {
        // format msg in case of arguments
        va_list args;
        va_start(args, msg);
        vsnprintf(buff, BUFF_SIZE, msg, args);
        va_end(args);

        // log message
        log_msg(LOG_DEBUG, buff);
    }
}

void log_trace(const char* msg, ...) {
    if (log_level == Trace && log_file != NULL) {
        // format msg in case of arguments
        va_list args;
        va_start(args, msg);
        vsnprintf(buff, BUFF_SIZE, msg, args);
        va_end(args);

        // log message
        log_msg(LOG_TRACE, buff);
    }
}
