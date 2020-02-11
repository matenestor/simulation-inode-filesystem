#ifndef FORMAT_H
#define FORMAT_H


#include <ctype.h>
#include <time.h>

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

#define mb2b(mb)      ((mb)*1024*1024)
#define isnegnum(cha) (cha[0] == '-')
#define isinrange(n)  ((n) > 0 && (n) <= FS_SIZE_MAX)

static inline bool isnumeric(const char* num) {
    bool is_num = true;

    for (size_t i = 0; i < strlen(num); ++i) {
        if (!isdigit(num[i])) {
            is_num = false;
        }
    }

    return is_num;
}


static inline void get_datetime(char* datetime) {
    time_t t;
    struct tm* tm_info;

    // get time
    t = time(NULL);
    tm_info = localtime(&t);

    strftime(datetime, DATETIME_LENGTH, "%Y-%m-%d %H:%M:%S", tm_info);
}


#endif
