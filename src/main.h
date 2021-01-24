#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>

#define DEBUG 1

#define PR_INTRO	" _____________________________________________\n" \
					"|                                             |\n" \
					"| Welcome to simulator of i-nodes filesystem. |\n" \
					"| Created by matenestor for class KIV/ZOS.    |\n" \
					"|_____________________________________________|\n"

#define PR_HELP		"Usage: inodes <filesystem-name>\n" \

// simulation running status for signal handler
extern bool is_running;

extern void run();
extern int init_simulation(int, char const **);
extern void close_filesystem();

#endif
