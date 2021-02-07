#ifndef LOGGER_API_H
#define LOGGER_API_H

#include <stdbool.h>

// levels of severity
enum log_level {
	Log_Off			= 0,
	Log_Critical	= 1,
	Log_Error		= 2,
	Log_Warning		= 3,
	Log_Info		= 4,
	Log_Debug		= 5,
	Log_Trace		= 6
};

bool logger_init();
void logger_destroy();
void logger_set_level(enum log_level);
void log_critical(const char*, ...);
void log_error(const char*, ...);
void log_warning(const char*, ...);
void log_info(const char*, ...);
void log_debug(const char*, ...);
void log_trace(const char*, ...);


#endif
