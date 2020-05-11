#ifndef FORMAT_H
#define FORMAT_H


#define DATETIME_LENGTH 25

/** Maximal filesystem size in megabytes MB. (because using 32b integers) */
#define FS_SIZE_MAX     4095
/** Filesystem's clusters size in bytes B. */
#define FS_CLUSTER_SIZE 1024
/** Percentage of space for data in filesystem */
#define PERCENTAGE      0.95

#define mb2b(mb)      ((mb)*1024UL*1024UL)
#define isnegnum(cha) (cha[0] == '-')
#define isinrange(n)  ((n) > 0 && (n) <= FS_SIZE_MAX)


#endif
