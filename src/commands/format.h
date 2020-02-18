#ifndef FORMAT_H
#define FORMAT_H


#define DATETIME_LENGTH 25

/** Maximal filesystem size in megabytes MB. */
#define FS_SIZE_MAX     1024
/** Filesystem's clusters size in bytes B. */
#define FS_CLUSTER_SIZE 1024
/** Percentage of space for data in filesystem */
#define PERCENTAGE      0.9

#define SIGNATURE "kmat95"

#define mb2b(mb)      ((mb)*1024*1024)
#define isnegnum(cha) (cha[0] == '-')
#define isinrange(n)  ((n) > 0 && (n) <= FS_SIZE_MAX)


#endif

// TODO improve
//  bigger filesystem size, than 1 GB
//  changeable size of clusters, not only 1 kB
