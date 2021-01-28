#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "main.h"

#include "logger.h"
#include "errors.h"


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

	puts(PR_INTRO);
	if (init_simulation(argc, argv) == RETURN_SUCCESS) {
		run();
	} else {
		puts(PR_HELP);
		log_fatal("Terminating: %s", my_strerror(my_errno));
	}

	// destroy logger
	logger_destroy();

	return 0;
}
