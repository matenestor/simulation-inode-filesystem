#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "inc/fs_prompt.h"
#include "inc/inode.h"

// input buffer size for user
#define BUFF_IN_LENGTH  1024

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
                 "  - Note, that commands will accept only exact count of arguments as written above, not more.\n" \
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
#define CMD_DEBUG    "debug"

#define isoverflow(c)          ((c) != '\n' && (c) != '\0')
#define BUFF_CLR(dest, count)  memset(dest, '\0', count)

/** Is filesystem formatted or not. */
static bool is_formatted;
/** Is simulation running or not. */
bool is_running;

/** Filesystem name given by user. Does not change during runtime. */
char fs_name[STRLEN_FSNAME];
/** Filesystem pwd buffer. */
char buff_pwd[BUFF_PWD_LENGTH];
/** Current working directory prompt in console. */
char buff_prompt[BUFF_PROMPT_LENGTH];

/** Filesystem file, which is being worked with. */
FILE* filesystem;

/** Super block of actual using filesystem. */
struct superblock sb = {0};
/** Inode, where user currently is. */
struct inode in_actual = {0};

extern int init_filesystem(const char*, bool*);
extern void close_filesystem();

extern int cp_(const char*, const char*);
extern int mv_(const char*, const char*);
extern int rm_(const char*);
extern int mkdir_(const char*);
extern int rmdir_(const char*);
extern int ls_(const char*);
extern int cat_(const char*);
extern int cd_(const char*);
extern int pwd_();
extern int info_(const char*);
extern int incp_(const char*, const char*);
extern int outcp_(const char*, const char*);
extern int load_(const char*);
extern int format_(const char*, const char*);
extern int fsck_();
extern int tree_(const char*);
extern int debug_(const char*);

#endif
