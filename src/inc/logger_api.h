#ifndef LOGGER_API_H
#define LOGGER_API_H


/** Levels of severity. */
enum __level {
    Log_Off     = 0,
    Log_Fatal   = 1,
    Log_Error   = 2,
    Log_Warning = 3,
    Log_Info    = 4,
    Log_Debug   = 5,
    Log_Trace   = 6
};

/** Initialize logger. */
int logger_init();
/** Destroy logger. */
void logger_destroy();
/** Set level of logger severity. */
void logger_set_level(enum __level);

// types of log messages
void log_fatal(const char*, ...);
void log_error(const char*, ...);
void log_warning(const char*, ...);
void log_info(const char*, ...);
void log_debug(const char*, ...);
void log_trace(const char*, ...);


#endif
