#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>

#define DEBUG 1

#define PR_HELP		"Usage: inodes <filesystem-name>\n" \

// simulation running status for signal handler
extern bool is_running;

extern void run();
extern int init_simulation(const char*);
extern void close_filesystem();

#endif
