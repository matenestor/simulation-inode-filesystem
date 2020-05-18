#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "format.h"
#include "../inc/fs_cache.h"
#include "../inc/inode.h"
#include "../inc/return_codes.h"
#include "../fs_operations.h"

#include "../inc/logger_api.h"
#include "../error.h"


static bool isnumeric(const char* num) {
    bool is_num = true;

    for (size_t i = 0; i < strlen(num); ++i) {
        if (!isdigit(num[i])) {
            is_num = false;
        }
    }

    return is_num;
}


static void get_datetime(char* datetime) {
    time_t t;
    struct tm* tm_info;

    // get time
    t = time(NULL);
    tm_info = localtime(&t);

    strftime(datetime, DATETIME_LENGTH, "%Y-%m-%d %H:%M:%S", tm_info);
}


/******************************************************************************
 *
 * 	Check if given string by user before formatting is in correct format and size.
 *
 */
static int is_valid_size(const char* num_str, size_t* size) {
    int ret = RETURN_FAILURE;
    long num = 0;

    // if string with number was given
    if (strlen(num_str) > 0) {
        // if number in given string is not negative
        if (!isnegnum(num_str)) {
            // if number in given string is convertible to number
            // i made this function. because i don't like, that C converts even '12ab' to '12'
            if (isnumeric(num_str)) {
                // try to convert the number
                num = strtol(num_str, NULL, 10);

                // if number was converted successfully
                if (errno != ERANGE) {
                    // if number is in simulator range
                    if (isinrange(num)) {
                        // set size for the format command
                        *size = (size_t) num;
                        ret = RETURN_SUCCESS;
                    }
                    else {
                        set_myerrno(Err_fs_size_sim_range);
                        fprintf(stderr, "format: use range from 1 to %d [MB]\n", FS_SIZE_MAX);
                    }
                }
                else {
                    set_myerrno(Err_fs_size_sys_range);
                    perror("system error");
                    errno = 0;
                }
            }
            else {
                set_myerrno(Err_fs_size_nan);
            }
        }
        else {
            set_myerrno(Err_fs_size_negative);
        }
    }
    else {
        set_myerrno(Err_fs_size_none);
    }

    return ret;
}


static int init_superblock(const int size, const size_t clstr_cnt) {
    char datetime[DATETIME_LENGTH] = {0};
    get_datetime(datetime);

    printf("init: superblock.. ");

    // inode bitmap is after superblock
    uint32_t addr_bm_in = sizeof(struct superblock);
    // data bitmap is after inode bitmap
    uint32_t addr_bm_dat = addr_bm_in + clstr_cnt;
    // inodes are after data bitmap
    uint32_t addr_in = addr_bm_dat + clstr_cnt;
    // data are at the end of filesystem -- there is unused space between inodes and data
    uint32_t addr_dat = addr_in + clstr_cnt * sizeof(struct inode);

    // init superblock variables
    strncpy(sb.signature, "kmat95", sizeof(sb.signature) - 1);
    sprintf(sb.volume_descriptor, "%s, made by matenestor", datetime);
    sb.disk_size = size;
    sb.cluster_size = FS_CLUSTER_SIZE;
    sb.cluster_count = clstr_cnt;
    sb.count_links = sb.cluster_size / sizeof(int32_t);
    sb.count_dir_items = sb.cluster_size / sizeof(struct directory_item);
    sb.addr_bm_inodes = addr_bm_in;
    sb.addr_bm_data = addr_bm_dat;
    sb.addr_inodes = addr_in;
    sb.addr_data = addr_dat;

    // write superblock to file
    fs_write_superblock(&sb, sizeof(struct superblock), 1);

    fs_flush();

    puts("done");

    return RETURN_SUCCESS;
}


static int init_bitmap(const size_t clstr_cnt) {
    size_t i;
    size_t loops = clstr_cnt / CACHE_SIZE;
    size_t over_fields = clstr_cnt % CACHE_SIZE;
    // count of field to be read
    size_t batch = loops > 0 ? CACHE_SIZE : over_fields;
    bool bitmap[batch];

    printf("init: bitmap.. ");

    // first loop is done manually, because very first field is set to 'false'
    memset(bitmap, true, batch);
    bitmap[0] = false;
    fs_write_bool(bitmap, sizeof(bool), batch);
    bitmap[0] = true;

    // init rest of the bitmap
    for (i = 1; i <= loops; ++i) {
        batch = i < loops ? CACHE_SIZE : over_fields;
        fs_write_bool(bitmap, sizeof(bool), batch);
    }

    fs_flush();

    puts("done");

    return RETURN_SUCCESS;
}


static int init_inodes(const size_t clstr_cnt) {
    size_t i, j;
    // how many inodes can be read into 'CACHE_SIZE'
    size_t cache_capacity = CACHE_SIZE / sizeof(struct inode);
    // clstr_cnt == total inodes count
    size_t loops = clstr_cnt / cache_capacity;
    size_t over_inodes = clstr_cnt % cache_capacity;
    // count of inodes to be read
    size_t batch = loops > 0 ? cache_capacity : over_inodes;
    // inode template
    struct inode in;
    // array of cached inode in filesystem (inodes count == cluster count)
    struct inode inodes[cache_capacity];

    printf("init: inodes.. ");

    /* START set root inode */

    // init root inode
    in.id_inode = 0;
    // first inode is used for root directory during format
    in.item_type = Itemtype_directory;
    // file size for directories is size of data cluster
    in.file_size = FS_CLUSTER_SIZE;
    // root inode point to data cluster on index 0
    in.direct[0] = 0;
    // other links are free
    for (i = 1; i < COUNT_DIRECT_LINKS; ++i) {
        in.direct[i] = FREE_LINK;
    }
    // set indirect links level 1
    for (i = 0; i < COUNT_INDIRECT_LINKS_1; ++i) {
        in.indirect1[i] = FREE_LINK;
    }
    // set indirect links level 2
    for (i = 0; i < COUNT_INDIRECT_LINKS_2; ++i) {
        in.indirect2[i] = FREE_LINK;
    }

    // cache root inode to local array for future FS_WRITE
    memcpy(&inodes[0], &in, sizeof(struct inode));
    // and to simulator cache
    memcpy(&in_actual, &in, sizeof(struct inode));

    /* END set root inode */

    /* START set free inodes */

    // reset values for rest of the inodes
    in.item_type = Itemtype_free;
    in.file_size = 0;
    in.direct[0] = FREE_LINK;

    // cache rest of inodes to local array for future FS_WRITE
    for (i = 1; i < batch; ++i) {
        in.id_inode = i;
        memcpy(&inodes[i], &in, sizeof(struct inode));
    }

    /* END set free inodes */

    // first loop is done manually, because very first inode is root
    fs_write_inode(inodes, sizeof(struct inode), batch);

    // rewrite first 'root' inode in cache array to free inode
    memcpy(&inodes[0], &in, sizeof(struct inode));

    // write rest of the inodes
    for (i = 1; i <= loops; ++i) {
        batch = i < loops ? cache_capacity : over_inodes;

        // initialize new inode ids
        for (j = 0; j < batch; ++j) {
            // really 'cache_capacity', not 'batch'
            inodes[j].id_inode = j + i*cache_capacity;
        }

        fs_write_inode(inodes, sizeof(struct inode), batch);
    }

    fs_flush();

    puts("done");

    return RETURN_SUCCESS;
}


static int init_clusters(const uint32_t fs_size) {
    size_t i;
    // how much bytes is missing till end of filesystem
    // after meta part -- empty space part + data part
    uint32_t remaining_part = mb2b(fs_size) - ftell(filesystem);
    // helper array to be filled from
    char zeros[CACHE_SIZE] = {0};

    // variables for printing percentage, so  it doesn't look like nothing is happening
    size_t loops = remaining_part / CACHE_SIZE;
    size_t percent5 = loops / 20 + 1;

    printf("init: data clusters.. 0 %%");

    // fill rest of filesystem with batches of zeros
    for (i = 1; i <= loops; ++i) {
        fs_write_char(zeros, sizeof(char), CACHE_SIZE);

        if (i % percent5 == 0)
            printf("\rinit: data clusters.. %d %%", (size_t) (((float) i / loops)*100));
    }
    // fill zeros left till very end of filesystem (over fields)
    fs_write_char(zeros, sizeof(char), remaining_part % CACHE_SIZE);

    fs_flush();

    puts("\rinit: data clusters.. done");

    return RETURN_SUCCESS;
}


static int init_root_dir() {
    // root directory items
    struct directory_item di[2] = {0};

    // init directories in root directory (fk_id_inode is already set to 0 by init)
    strncpy(di[0].item_name, ".", 1);
    strncpy(di[1].item_name, "..", 2);

    // write root directory items to file
    fs_seek_set(sb.addr_data);
    fs_write_directory_item(di, sizeof(struct directory_item), 2);

    fs_flush();

    return RETURN_SUCCESS;
}


int format_(const char* fs_size_str, const char* path) {
    int ret = RETURN_FAILURE;
    // necessary step with double type variable for precision
    double dt_percentage = 0;
    uint32_t clstr_cnt = 0, fs_size = 0;

    log_info("Formatting filesystem [path: %s] [size: %s]", path, fs_size_str);

    if (is_valid_size(fs_size_str, &fs_size) == RETURN_SUCCESS) {
        // Count of clusters in data blocks part is equal to bitmaps sizes and count of inodes.
        // 'fs_size' is in MB, 'cluster_size' is in B, data part is 100*'PERCENTAGE' % of whole filesystem
        dt_percentage = mb2b(fs_size) * PERCENTAGE;
        clstr_cnt = (uint32_t) (dt_percentage / FS_CLUSTER_SIZE);

        if ((filesystem = fopen(path, "wb+")) != NULL) {
            // superblock
            init_superblock(fs_size, clstr_cnt);
            // bitmap of inodes
            init_bitmap(clstr_cnt);
            // bitmap of data blocks
            init_bitmap(clstr_cnt);
            // inodes
            init_inodes(clstr_cnt);
            // data clusters
            init_clusters(fs_size);
            // root directory
            init_root_dir();

            log_info("format: Filesystem [%s] with size [%d MB] formatted.", path, fs_size);
            printf("format: filesystem formatted, size: %d MB\n", fs_size);
            ret = RETURN_SUCCESS;
        }
        else {
            log_error("format: System error while formatting: %s", strerror(errno));
            perror("format");
            errno = 0;
        }
    }
    else {
        log_error("format: Simulation error while formatting: %s", my_strerror(my_errno));
        my_perror("format");
        reset_myerrno();
    }

    if (ret == RETURN_FAILURE) {
        set_myerrno(Err_fs_not_formatted);
        log_error("Filesystem could not be formatted. [size %s MB] [name: %s]", fs_size_str, fs_name);
    }

    return ret;
}
