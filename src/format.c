#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "inc/error.h"
#include "inc/format.h"
#include "inc/inode.h"
#include "inc/logger_api.h"
#include "inc/return_codes.h"


bool isnumeric(const char* num) {
    bool is_num = true;

    for (size_t i = 0; i < strlen(num); ++i) {
        if (!isdigit(num[i])) {
            is_num = false;
        }
    }

    return is_num;
}


void get_datetime(char* datetime) {
    time_t t;
    struct tm* tm_info;

    // get time
    t = time(NULL);
    tm_info = localtime(&t);

    // Mon Jan 01 00:00:00 2020
    strftime(datetime, DATETIME_LENGTH, "%Y-%m-%d %H:%M:%S", tm_info);
}


int is_valid_size(const char* num_str, int* size) {
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
                        *size = (int) num;
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


int init_superblock(FILE* filesystem, const int size) {
    char datetime[DATETIME_LENGTH] = {0};
    get_datetime(datetime);

    // size is in MB, cluster_size is in B
    // with cluster size 1024 B, there is obvious redundancy, but what if cluster size will change..?
    // data part is 90 % of whole filesystem
    int clstr_cnt = (int)(mb2b(size)*0.9) / FS_CLUSTER_SIZE;

    // inode bitmap is after superblock
    int addr_bm_in = sizeof(struct superblock);
    // data bitmap is after inode bitmap
    int addr_bm_dat = addr_bm_in + clstr_cnt;
    // inodes are after data bitmap
    int addr_in = addr_bm_dat + clstr_cnt;
    // data are at the end of filesystem -- there is unused space between inodes and data
    int addr_dat = mb2b(size) - (clstr_cnt*FS_CLUSTER_SIZE);

    // init superblock variables
    struct superblock sb = {SIGNATURE, "", size, FS_CLUSTER_SIZE, clstr_cnt, addr_bm_in, addr_bm_dat, addr_in, addr_dat};
    // init also volume_descriptor
    sprintf(sb.volume_descriptor, "%s, made by matenestor", datetime);

    // write superblock to file
    fwrite(&sb, sizeof(struct superblock), 1, filesystem);

    return RETURN_SUCCESS;
}


int format_(const char* size_str, FILE** filesystem, const char* path, struct inode* actual) {
    int ret = RETURN_FAILURE;
    int size = 0;

    log_info("Formatting filesystem [%s]", path);

    if (is_valid_size(size_str, &size) == RETURN_SUCCESS) {
        if ((*filesystem = fopen(path, "wb+")) != NULL) {
            init_superblock(*filesystem, size);

            log_info("Filesystem [%s] with size [%d] formatted.", path, size);
            printf("format: filesystem formatted, size: %d MB\n", size);
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
