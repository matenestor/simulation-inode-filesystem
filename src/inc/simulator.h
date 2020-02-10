#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "inode.h"
#include "fspath_info.h"


// since there is no standard way to flush stdin and making resizable input buffer
// for this project is overkill, let me just make the buffers big
#define BUFFIN_LENGTH  1024
#define BUFFOUT_LENGTH 1024
#define BUFF_PWD       64

// +3 for : > and space in FORMAT_PROMPT
#define STRLEN_PROMPT (STRLEN_FSNAME+BUFF_PWD+3)

// length of longest command available 'format' is 7 with \0
#define STRLEN_LONGEST_CMD 7

#define SEPARATOR     "/"
#define FORMAT_PROMPT "%s:%s> "

#define PR_TRY_HELP "Try 'help' for more information."

#define PR_USAGE "Available commands:\n" \
                 "  cp      SOURCE DEST       Copy SOURCE to DEST.\n" \
                 "  mv      SOURCE DEST       Rename SOURCE to DEST, or move SOURCE to DIRECTORY.\n" \
                 "  rm      FILE              Remove (unlink) the FILE.\n" \
                 "  mkdir   DIRECTORY         Create the DIRECTORY, if they do not already exist.\n" \
                 "  rmdir   DIRECTORY         Remove the DIRECTORY, if they are empty.\n" \
                 "  ls      [FILE]            List information about the FILE (the current directory by default).\n" \
                 "  cat     FILE              Concatenate FILE to standard output.\n" \
                 "  cd      [DIRECTORY]       Change the working DIRECTORY.\n" \
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
                 "  help                      Print this help.\n" \
                 "  exit                      Exit simulation.\n\n" \
                 "  - Note, that commands will accept only exact count of arguments as written above, not more." \
                 "    Other arguments are discarded. Arguments in [] brackets are optional.\n"

#define CMD_CP       "cp"
#define CMD_MV       "mv"
#define CMD_RM       "rm"
#define CMD_MKDIR    "mkdir"
#define CMD_RMDIR    "rmdir"
#define CMD_LS       "ls"
#define CMD_CAT      "cat"
#define CMD_CD       "cd"
#define CMD_PWD      "pwd"
#define CMD_INFO     "info"
#define CMD_INCP     "incp"
#define CMD_OUTCP    "outcp"
#define CMD_LOAD     "load"
#define CMD_FORMAT   "format"
#define CMD_FSCK     "fsck"
#define CMD_TREE     "tree"
#define CMD_HELP     "help"
#define CMD_EXIT     "exit"

#define isoverflow(c) ((c) != '\n' && (c) != '\0')

/** Filesystem name given by user. Does not change during runtime. */
static char fs_name[STRLEN_FSNAME];
/** Filesystem path. Does not change during runtime. */
static char fs_path[STRLEN_FSPATH];

/** Current working directory cache. Do change after every 'cd' command. */
char buff_pwd[BUFF_PWD];
/** Whole prompt in console. Do change after every pwd change. */
char buff_prompt[STRLEN_PROMPT];

/** Filesystem file, which is being worked with. */
FILE* filesystem;

/** Super block of actual using filesystem. */
struct superblock sb = {0};
/** Inode, where user currently is. */
struct inode in_actual = {0};
/** Inode used for commands -- cp, mv, incp, outcp. */
struct inode in_distant = {0};

extern int cp_(char*, char*);
extern int mv_(char*, char*);
extern int rm_(char*);
extern int mkdir_(char*);
extern int rmdir_(char*);
extern int ls_(char*);
extern int cat_(char*);
extern int cd_(char*);
extern int pwd_();
extern int info_(char*);
extern int incp_(char*, char*);
extern int outcp_(char*, char*);
extern int load_(char*);
extern int format_(char*, char*);
extern int fsck_();
extern int tree_(char*);


#endif
