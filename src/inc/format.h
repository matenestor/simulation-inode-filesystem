#ifndef FORMAT_H
#define FORMAT_H


#define DATETIME_LENGTH 25

/** Maximal filesystem size in megabytes MB. */
#define FS_SIZE_MAX     1024
/** Filesystem's clusters size in bytes B. */
#define FS_CLUSTER_SIZE 1024
/** Percentage of space for data in filesystem */
#define PERCENTAGE      0.9
/** Batch size for fwriting zeros to data block of filesystem */
#define BATCH_SIZE      8192

#define SIGNATURE "kmat95"

#define isnegnum(cha) (cha[0] == '-')
#define isinrange(n)  ((n) > 0 && (n) <= FS_SIZE_MAX)
#define mb2b(mb)      ((mb)*1024*1024)
#define kb2b(kb)      ((kb)*1024)

extern FILE* filesystem;
extern struct superblock sb;
extern struct inode actual;


#endif
