#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "fspath_info.h"


#define STRLEN_PWD    1024
#define STRLEN_PROMPT (STRLEN_FSNAME+STRLEN_PWD)

#define SEPARATOR     "/"
#define FORMAT_PROMPT "%s:%s> "

#define PR_USAGE "Available commands:\n" \
                 "  cp      SOURCE DEST       Copy SOURCE to DEST.\n" \
                 "  mv      SOURCE DEST       Rename SOURCE to DEST, or move SOURCE to DIRECTORY.\n" \
                 "  rm      FILE              Remove (unlink) the FILE.\n" \
                 "  mkdir   DIRECTORY         Create the DIRECTORY, if they do not already exist.\n" \
                 "  rmdir   DIRECTORY         Remove the DIRECTORY, if they are empty.\n" \
                 "  ls      FILE              List information about the FILE (the current directory by default).\n" \
                 "  cat     FILE              Concatenate FILE to standard output.\n" \
                 "  cd      DIRECTORY         Change the working DIRECTORY.\n" \
                 "  pwd                       Print the working directory.\n" \
                 "  info    FILE|DIRECTORY    Print information about FILE or DIRECTORY in format\n" \
                 "                            \"NAME-SIZE-inode NUMBER-direct/indirect links\".\n" \
                 "  incp    SOURCE DEST       Copy file from SOURCE on local HDD to DEST in filesystem.\n" \
                 "  outcp   SOURCE DEST       Copy file from SOURCE in filesystem to DEST on local HDD.\n" \
                 "  load    FILE              Load FILE with commands and start executing them (1 command = 1 line).\n" \
                 "  format  SIZE              Format filesystem of SIZE in megabytes.\n" \
                 "                            If filesystem already exists, the data inside will be destroyed.\n" \
                 "  fsck                      Check and repair a Linux filesystem.\n" \
                 "  tree    [DIRECTORY]       List contents of directories in a tree-like format.\n" \
                 "  help                      Print this help.\n\n"

/** Filesystem name given by user. */
static char fsname[STRLEN_FSNAME];
/** Filesystem path. */
static char fspath[STRLEN_FSPATH];

/** Current working directory cache. */
static char pwd[STRLEN_PWD];
/** Whole prompt in console. */
static char prompt[STRLEN_PROMPT];

#endif
