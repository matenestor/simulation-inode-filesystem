#ifndef FS_PROMPT_H
#define FS_PROMPT_H

// max length of filesystem name
#define STRLEN_FSNAME	32
// max length of filesystem path
#define STRLEN_FSPATH   1024
// max. length of pwd in prompt
#define STRLEN_PWD      64
// buffer size of pwd
#define BUFF_PWD_LENGTH 1024
// +3 for ":> " in FORMAT_PROMPT
#define BUFF_PROMPT_LENGTH  (STRLEN_FSNAME + STRLEN_PWD + 3)

#define FORMAT_PROMPT "%s:%s> "
#define SEPARATOR     "/"

#endif
