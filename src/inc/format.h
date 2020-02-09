#ifndef FORMAT_H
#define FORMAT_H


#define DATETIME_LENGTH 25

// maximal filesystem size in megabytes MB
#define FS_SIZE_MAX     1024
// filesystem's clusters size in bytes B
#define FS_CLUSTER_SIZE 1024

#define SIGNATURE "kmat95"

#define isnegnum(cha) (cha[0] == '-')
#define isinrange(n) ((n)>0&&(n)<FS_SIZE_MAX)
#define mb2b(mb) ((mb)*1024*1024)

#endif
