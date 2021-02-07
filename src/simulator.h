#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdio.h>
#include <stdbool.h>

#include "fs_prompt.h"
#include "inode.h"

#define PR_USAGE 	"Available commands:\n" \
					"  format  SIZE              Format filesystem of SIZE in megabytes (MB).\n" \
					"                            If filesystem already exists, the data inside will be destroyed.\n" \
					"  pwd                       Print the working directory.\n" \
					"  cat     FILE              Concatenate FILE to standard output.\n" \
					"  ls      [FILE]            List information about the FILE (the current directory by default).\n" \
					"  info    FILE|DIRECTORY    Print information about FILE or DIRECTORY in format\n" \
					"                            \"NAME-SIZE-inode NUMBER-direct/indirect links\".\n" \
					"  mv      SOURCE DEST       Rename SOURCE to DEST, or move SOURCE to DIRECTORY.\n" \
					"  cp      SOURCE DEST       Copy SOURCE to DEST.\n" \
					"  rm      FILE              Remove (unlink) the FILE.\n" \
					"  cd      [DIRECTORY]       Change the working DIRECTORY.\n" \
					"  mkdir   DIRECTORY         Create the DIRECTORY, if they do not already exist.\n" \
					"  rmdir   DIRECTORY         Remove the DIRECTORY, if they are empty.\n" \
					"  incp    SOURCE DEST       Copy file from SOURCE on local HDD to DEST in filesystem.\n" \
					"  outcp   SOURCE DEST       Copy file from SOURCE in filesystem to DEST on local HDD.\n" \
					"  load    FILE              Load FILE with commands and start executing them (1 command = 1 line).\n" \
					"  fsck                      Check and repair the filesystem.\n" \
					"  help                      Print this help.\n" \
					"  exit                      Exit simulation.\n\n" \
					"  - Commands will accept only exact count of arguments as written above, not more.\n" \
					"    Other arguments are discarded. Arguments in [] brackets are optional.\n"

// input buffer size for user
#define BUFFER_INPUT_LENGTH			2048 // enough space to fill 80x24 screen and a bit more
#define BUFFER_CLEAR(dest, count)	memset(dest, '\0', count)
#define isoverflow(c)				(!((c) == '\n' || (c) == '\0'))

bool is_formatted;		// is filesystem formatted or not
bool is_running;		// is simulation running or not

char fs_name[STRLEN_FS_NAME];			// fs name given by user
char buff_pwd[STRLEN_PWD_LENGTH];		// fs current working directory
char buff_prompt[BUFFER_PROMPT_LENGTH];	// prompt in console

// filesystem file, which is being worked with
FILE* filesystem;

// super block of actual using filesystem
struct superblock sb = {0};
// inode, where user currently is
struct inode in_actual = {0};

extern int sim_pwd();
extern int sim_cat(const char*);
extern int sim_ls(const char*);
extern int sim_info(const char*);
extern int sim_mv(const char*, const char*);
extern int sim_cp(const char*, const char*);
extern int sim_rm(const char*);
extern int sim_cd(const char*);
extern int sim_mkdir(const char*);
extern int sim_rmdir(const char*);
extern int sim_incp(const char*, const char*);
extern int sim_outcp(const char*, const char*);
extern int sim_load(const char*);
extern int sim_fsck();
extern int sim_format(const char*, const char*);
extern int sim_debug(const char*);

#endif
