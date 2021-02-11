#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "main.h"

#include "logger.h"
#include "errors.h"

// TODO longterm fs_config.h
// TODO longterm error messages with arguments given
// TODO longterm more meaningful logger messages -- with reason of fail
// TODO longterm malloc in format.c, instead of CACHE_SIZE
// TODO longterm sort ls by item type and by alphabet

void signal_handler(int signum) {
	// with ctrl+C, program has to be terminated here, because else it waits for input
	// in simulator.c > handle_input(...) on fgets(...)
	// is there any way not to end it here?
	close_filesystem();
	is_running = false;
	set_myerrno(Err_signal_interrupt);
	log_info("Terminating: %s", my_strerror(my_errno));
	exit(EXIT_SUCCESS);
}

int main(int argc, char const **argv) {
	#if DEBUG
	// Clion debugger output
	setbuf(stdout, 0);
	setbuf(stderr, 0);
	#endif

	// init logger and set level
	if (logger_init()) {
		#if DEBUG
		logger_set_level(Log_Debug);
		#else
		logger_set_level(Log_Info);
		#endif
	}
	// error initialization
	reset_myerrno();
	// register signal interrupt
	signal(SIGINT, signal_handler);

	if (argc > 1) {
		if (init_simulation(argv[1]) == RETURN_SUCCESS) {
			run();
		} else {
			puts(PR_HELP);
			log_critical("Terminating: %s", my_strerror(my_errno));
		}
	} else {
		set_myerrno(Err_fs_name_missing);
		my_perror("zos");
		log_error("Terminating: %s", my_strerror(my_errno));
	}

	// destroy logger
	logger_destroy();

	return 0;
}
