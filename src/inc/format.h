#ifndef FORMAT_H
#define FORMAT_H

#include <sys/types.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


#define DATETIME_LENGTH 25
#define FS_SIZE_MAX 1024

#define SIGNATURE "kmat95"

#define isnegnum(cha) (cha[0] == '-')
#define isinrange(n) ((n)>0&&(n)<FS_SIZE_MAX)


#endif
