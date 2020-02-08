#include <errno.h>
#include <stdlib.h>

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
    strftime(datetime, DATETIME_LENGTH, "%c", tm_info);
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
                        *size = num;
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


int format_(const char* size_str, FILE** filesystem, const char* path) {
    int ret = RETURN_FAILURE;
    int size = 0;
    char datetime[DATETIME_LENGTH] = {0};

    log_info("Formatting filesystem [%s]", path);

    if (is_valid_size(size_str, &size) == RETURN_SUCCESS) {
        get_datetime(datetime);

        struct superblock sb = {SIGNATURE, "", 10, 1024, 10, 1, 2, 3, 4};
        sprintf(sb.volume_descriptor, "%s, made by matenestor", datetime);

        // filesystem is ready to be loaded
        if ((*filesystem = fopen(path, "wb+")) != NULL) {
            // TODO write structures to file
            fwrite(&sb, sizeof(struct superblock), 1, *filesystem);
            puts("Filesystem formatted.");
            ret = RETURN_SUCCESS;
        }
        else {
            log_error("format: %s", strerror(errno));
            perror("format");
            errno = 0;
        }
    }
    else {
        log_error("format: %s", my_strerror(my_errno));
        my_perror("format");
        reset_myerrno();
    }

    if (ret == RETURN_FAILURE) {
        set_myerrno(Fs_not_formatted);
    }

    return ret;
}
