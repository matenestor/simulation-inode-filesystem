#ifndef MAIN_H
#define MAIN_H


#define PR_INTRO " _____________________________________________\n" \
                 "|                                             |\n" \
                 "| Welcome to simulator of i-nodes filesystem. |\n" \
                 "| Created by matenestor for class KIV/ZOS.    |\n" \
                 "|_____________________________________________|\n\n"

#define PR_HELP  "Usage: inodes <filesystem-name>\n\n"

#define isunscr(c) ((c)==('_'))
#define isdot(c)   ((c)==('.'))

extern void load(const char*);
extern void run();


#endif
