#ifndef LOGGER_API_H
#define LOGGER_API_H


/** Levels of severity. */
typedef enum {
    Off     = 0,
    Fatal   = 1,
    Error   = 2,
    Warning = 3,
    Info    = 4,
    Debug   = 5,
    Trace   = 6
} level;

/** Initialize logger. */
int logger_init();
/** Destroy logger. */
void logger_destroy();
/** Set level of logger severity. */
void logger_set_level(level);

// types of log messages
void log_fatal(const char*, ...);
void log_error(const char*, ...);
void log_warning(const char*, ...);
void log_info(const char*, ...);
void log_debug(const char*, ...);
void log_trace(const char*, ...);


#endif
