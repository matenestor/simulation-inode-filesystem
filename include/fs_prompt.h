#ifndef FS_PROMPT_H
#define FS_PROMPT_H

// maximal length of filesystem name
#define STRLEN_FS_NAME		31

// buffer size of pwd
#define STRLEN_PWD_LENGTH	101

// +3 for ":> " in FORMAT_PROMPT
#define BUFFER_PROMPT_LENGTH 	(STRLEN_FS_NAME + STRLEN_PWD_LENGTH + 3)

#define FORMAT_PROMPT "%s:%s> "
#define SEPARATOR     "/"

#endif
