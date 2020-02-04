#ifndef MAIN_H
#define MAIN_H


#define PR_INTRO " _____________________________________________\n" \
                 "|                                             |\n" \
                 "| Welcome to simulator of i-nodes filesystem. |\n" \
                 "| Created by matenestor for class KIV/ZOS.    |\n" \
                 "|_____________________________________________|\n\n"

#define PR_HELP "Usage: inodes <filesystem-name>\n\n"

// also defined in simulator.h (note: length mentioned in doc comment of parse_fsname()
#define LENGTH_FSNAME_STRING 32

#define isunscr(c) ((c)==('_'))

extern void load(const char*, size_t);
extern void run();


#endif
