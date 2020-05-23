#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../inc/return_codes.h"
#include "../fs_operations.h"
#include "../utils.h"

#include "../inc/logger_api.h"
#include "../error.h"


static int print_links(const char* type, const int32_t* src, const size_t size) {
    size_t i;

    printf("%s", type);

    for (i = 0; i < size; ++i) {
        if (src[i] != FREE_LINK) {
            printf(" %d", src[i]);
        }
    }

    puts("");

    return 0;
}

int info_(const char* item) {
    int ret = RETURN_FAILURE;
    char item_name[STRLEN_ITEM_NAME] = {0};
    struct inode in_item = {0};

    if (strlen(item) > 0) {
        if (parse_name(item_name, item, STRLEN_ITEM_NAME) != RETURN_FAILURE) {
            if (get_inode_by_path(&in_item, item) != RETURN_FAILURE) {
                printf(" type: %s\n", in_item.item_type == Itemtype_directory ? "directory" : "file");
                printf(" name: %s\n", item_name);
                printf(" size: %d B = %.2f kB = %.2f MB\n", in_item.file_size, (double) in_item.file_size / 1024, (double) in_item.file_size / 1024 / 1024);
                printf(" inode id: %d\n", in_item.id_inode);

                print_links(" direct links:", in_item.direct, COUNT_DIRECT_LINKS);
                print_links(" indrct links lvl 1:", in_item.indirect1, COUNT_INDIRECT_LINKS_1);
                print_links(" indrct links lvl 2:", in_item.indirect2, COUNT_INDIRECT_LINKS_2);
                puts("");

                ret = RETURN_SUCCESS;
            }
        }
    }
    else {
        set_myerrno(Err_arg_missing);
        log_warning("info: unable to print info [%s]", item);
    }

    return ret;
}
