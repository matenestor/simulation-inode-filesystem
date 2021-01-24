#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>

#include "../include/logger.h"

#define LOG_BUFF_SIZE_			1024 			// size of buffer for logging
#define LOG_DATETIME_LENGTH_	22				// length of datetime string + \0
#define LOG_FNAME_				"inode.log"		// log file name

#define LOG_FATAL_		"[FATAL]  "
#define LOG_ERROR_		"[ERROR]  "
#define LOG_WARNING_	"[WARNING]"
#define LOG_INFO_		"[INFO]   "
#define LOG_DEBUG_		"[DEBUG]  "
#define LOG_TRACE_		"[TRACE]  "


static char buff[LOG_BUFF_SIZE_];	// buffering messages for logging
static enum log_level log_level;	// level of logger severity
static FILE* log_file;				// logging file


bool logger_init() {
	if ((log_file = fopen(LOG_FNAME_, "w")) == NULL) {
		fputs("Log file could not be opened. Log messages will not be writen.\n", stderr);
		return false;
	}
	else {
		logger_set_level(Log_Info);
		log_info("Logger initialized.");
		return true;
	}
}

void logger_destroy() {
	if (log_file != NULL) {
		log_info("Closing logger.");
		fflush(log_file);
		fclose(log_file);
	}
}

void logger_set_level(enum log_level lvl) {
	log_level = lvl;
}

static void get_datetime(char* datetime) {
	static time_t t;
	static struct tm* tm_info;

	// get time
	t = time(NULL);
	tm_info = localtime(&t);
	strftime(datetime, LOG_DATETIME_LENGTH_, "[%Y-%m-%d %H:%M:%S]", tm_info);
}

static void log_msg(const char* severity) {
	static char datetime[LOG_DATETIME_LENGTH_];
	get_datetime(datetime);

	// print log message to log file
	fprintf(log_file, "%s %s %s\n", severity, datetime, buff);
	fflush(log_file);
}

// ======   LOG MESSAGES   ====================================================

void log_fatal(const char* msg, ...) {
	if (log_level >= Log_Fatal && log_file != NULL) {
		va_list args;
		va_start(args, msg);
		vsnprintf(buff, LOG_BUFF_SIZE_, msg, args);
		va_end(args);
		log_msg(LOG_FATAL_);
	}
}

void log_error(const char* msg, ...) {
	if (log_level >= Log_Error && log_file != NULL) {
		va_list args;
		va_start(args, msg);
		vsnprintf(buff, LOG_BUFF_SIZE_, msg, args);
		va_end(args);
		log_msg(LOG_ERROR_);
	}
}

void log_warning(const char* msg, ...) {
	if (log_level >= Log_Warning && log_file != NULL) {
		va_list args;
		va_start(args, msg);
		vsnprintf(buff, LOG_BUFF_SIZE_, msg, args);
		va_end(args);
		log_msg(LOG_WARNING_);
	}
}

void log_info(const char* msg, ...) {
	if (log_level >= Log_Info && log_file != NULL) {
		va_list args;
		va_start(args, msg);
		vsnprintf(buff, LOG_BUFF_SIZE_, msg, args);
		va_end(args);
		log_msg(LOG_INFO_);
	}
}

void log_debug(const char* msg, ...) {
	if (log_level >= Log_Debug && log_file != NULL) {
		va_list args;
		va_start(args, msg);
		vsnprintf(buff, LOG_BUFF_SIZE_, msg, args);
		va_end(args);
		log_msg(LOG_DEBUG_);
	}
}

void log_trace(const char* msg, ...) {
	if (log_level == Log_Trace && log_file != NULL) {
		va_list args;
		va_start(args, msg);
		vsnprintf(buff, LOG_BUFF_SIZE_, msg, args);
		va_end(args);
		log_msg(LOG_TRACE_);
	}
}
