#include <sys/types.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/error.h"
#include "../inc/logger_api.h"

#include "../inc/format.h"
#include "../inc/fs_cache.h"
#include "../inc/fs_operations.h"
#include "../inc/inode.h"
#include "../inc/return_codes.h"


int is_valid_size(const char* num_str, size_t* size) {
    int ret = RETURN_FAILURE;
    long num = 0;

    // if string with number was given
    if (strlen(num_str) > 0) {
        // if number in given string is not negative
        if (!isnegnum(num_str)) {
            // if number in given string is convertible to number
            // i made this function. because i don't like C converts even '12ab' to '12'
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
                        printf("use range from 1 MB to %d MB\n", FS_SIZE_MAX);
                        set_myerrno(Fs_size_sim_range);
                    }
                }
                else {
                    set_myerrno(Fs_size_sys_range);
                    perror("system error");
                    errno = 0;
                }
            }
            else {
                set_myerrno(Fs_size_nan);
            }
        }
        else {
            set_myerrno(Fs_size_negative);
        }
    }
    else {
        set_myerrno(Fs_size_none);
    }

    return ret;
}


int init_superblock(const int size, const size_t clstr_cnt) {
    char datetime[DATETIME_LENGTH] = {0};
    get_datetime(datetime);

    // inode bitmap is after superblock
    int32_t addr_bm_in = sizeof(struct superblock);
    // data bitmap is after inode bitmap
    int32_t addr_bm_dat = addr_bm_in + clstr_cnt;
    // inodes are after data bitmap
    int32_t addr_in = addr_bm_dat + clstr_cnt;
    // data are at the end of filesystem -- there is unused space between inodes and data
    int32_t addr_dat = mb2b(size) - (clstr_cnt * FS_CLUSTER_SIZE);

    // init superblock variables
    strcpy(sb.signature, SIGNATURE);
    sprintf(sb.volume_descriptor, "%s, made by matenestor", datetime);
    sb.disk_size = size;
    sb.cluster_size = FS_CLUSTER_SIZE;
    sb.cluster_count = clstr_cnt;
    sb.addr_bm_inodes = addr_bm_in;
    sb.addr_bm_data = addr_bm_dat;
    sb.addr_inodes = addr_in;
    sb.addr_data = addr_dat;

    // write superblock to file
    FS_WRITE(&sb, sizeof(struct superblock), 1);

    return RETURN_SUCCESS;
}


int init_bitmap(const size_t clstr_cnt) {
    size_t i;
    bool t = true;
    bool f = false;

    // first block is used for root directory during format
    FS_WRITE(&f, sizeof(bool), 1);

    // write rest of bitmap of inodes to file
    for (i = 1; i < clstr_cnt; ++i) {
        FS_WRITE(&t, sizeof(bool), 1);
    }

    return RETURN_SUCCESS;
}


int init_inodes(const size_t clstr_cnt) {
    size_t i;
    struct inode in;

    // init root inode
    in.id_node = 0;
    // first inode is used for root directory during format
    in.item_type = Item_directory;
    // file sizes for directories are 0
    in.file_size = 0;
    // root inode point to data cluster on index 0
    in.direct1 = 0;
    // other links are free
    in.direct2 = FREE_LINK;
    in.direct3 = FREE_LINK;
    in.direct4 = FREE_LINK;
    in.direct5 = FREE_LINK;
    in.indirect1 = FREE_LINK;
    in.indirect2 = FREE_LINK;

    // write root inode to file
    FS_WRITE(&in, sizeof(struct inode), 1);

    // cache root inode
    memcpy(&in_actual, &in, sizeof(struct inode));

    // reset values for rest of the inodes
    in.item_type = Item_free;
    in.direct1 = FREE_LINK;

    // init rest of inodes in filesystem
    for (i = 1; i < clstr_cnt; ++i) {
        in.id_node = i;
        FS_WRITE(&in, sizeof(struct inode), 1);
    }

    return RETURN_SUCCESS;
}


int init_clusters(const size_t fs_size) {
    size_t i;
    // how much bytes is missing till end of filesystem
    size_t diff = fs_size - FS_TELL;
    // helper array to be filled from
    char zeros[BATCH_SIZE] = {0};

    // note: this filling with batches is faster, than filling it one char by one
    // fill rest of filesystem with batches of zeros
    for (i = 0; i < diff / BATCH_SIZE; ++i) {
        FS_WRITE(zeros, sizeof(char), BATCH_SIZE);
    }
    // fill zeros left till very end of filesystem
    FS_WRITE(zeros, sizeof(char), diff % BATCH_SIZE);

    // move fs pointer to the beginning of data clusters
    FS_SEEK_SET(sb.addr_data);
    // init root directory item
    struct directory_item di = {SEPARATOR, 0};
    // write root directory to file
    FS_WRITE(&di, sizeof(struct directory_item), 1);

    // init this directory in root directory and write it also
    strcpy(di.item_name, ".");
    FS_WRITE(&di, sizeof(struct directory_item), 1);

    return RETURN_SUCCESS;
}


int format_(const char* fs_size_str, const char* path) {
    int ret = RETURN_FAILURE;
    size_t fs_size = 0;

    log_info("Formatting filesystem [%s]", path);

    if (is_valid_size(fs_size_str, &fs_size) == RETURN_SUCCESS) {
        if ((FS_VARIABLE_NAME = fopen(path, "wb+")) != NULL) {

            // count of clusters in data block is also used as bitmaps sizes and count of inodes
            // size is in MB, cluster_size is in B
            // with cluster size 1024 B, there is obvious redundancy, but what if cluster size will change..?
            // data part is 90 % of whole filesystem
            const size_t clstr_cnt = (size_t) (mb2b(fs_size) * PERCENTAGE) / FS_CLUSTER_SIZE;

            // superblock
            init_superblock(fs_size, clstr_cnt);
            // bitmap of inodes
            init_bitmap(clstr_cnt);
            // bitmap of data blocks
            init_bitmap(clstr_cnt);
            // inodes
            init_inodes(clstr_cnt);
            // data clusters
            init_clusters(mb2b(fs_size));

            // TODO create root folder / with mkdir, not manually

            // move fs pointer (hdd head) to the beginning
            FS_SEEK_SET(0);

            log_info("Filesystem [%s] with size [%d] formatted.", path, fs_size);
            printf("format: filesystem formatted, size: %zu MB\n", fs_size);
            ret = RETURN_SUCCESS;
        }
        else {
            log_error("System error while formatting: %s", strerror(errno));
            perror("format");
            errno = 0;
        }
    }
    else {
        log_error("Simulation error while formatting: %s", my_strerror(my_errno));
        my_perror("format");
        reset_myerrno();
    }

    if (ret == RETURN_FAILURE) {
        set_myerrno(Fs_not_formatted);
    }

    return ret;
}
