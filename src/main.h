#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>


#define DEBUG 1

#define PR_INTRO " _____________________________________________\n" \
                 "|                                             |\n" \
                 "| Welcome to simulator of i-nodes filesystem. |\n" \
                 "| Created by matenestor for class KIV/ZOS.    |\n" \
                 "|_____________________________________________|\n"

#define PR_HELP  "Usage: inodes <filesystem-name>\n"

#define isunscr(c)   ((c)==('_'))
#define isdot(c)     ((c)==('.'))
#define isslash(c)   ((c)==('/'))

/** Simulation running status for signal handler. */
extern bool is_running;

extern int init_simulation(const char*);
extern void close_filesystem();

#endif
