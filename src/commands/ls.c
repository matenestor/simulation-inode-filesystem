//#include <stdlib.h>
#include <string.h>

#include "../inc/logger_api.h"
#include "../error.h"

#include "../inc/return_codes.h"
#include "../inc/inode.h"
#include "../inc/fs_cache.h"
#include "../fs_operations.h"
#include "../utils.h"


static int list_direct_links(const int32_t* links) {
    size_t i, j;
    size_t items = 0;
    // cache inode for determination of file/directory
    struct inode in_ls;
    // cluster of item records to be printed
    struct directory_item cluster[sb.count_dir_items];

    for (i = 0; i < COUNT_DIRECT_LINKS; ++i) {
        // cache cluster with directory records pointed to by direct link
        if (links[i] != FREE_LINK) {
            FS_SEEK_SET(sb.addr_data + links[i] * sb.cluster_size);
            FS_READ(cluster, sizeof(struct directory_item), sb.count_dir_items);
            items = get_count_dirs(cluster);

            // print names of records (directory + files)
            for (j = 0; j < items; ++j) {
                FS_SEEK_SET(sb.addr_inodes + cluster[j].fk_id_inode * sizeof(struct inode));
                FS_READ(&in_ls, sizeof(struct inode), 1);

                if (in_ls.item_type == Itemtype_directory) {
                    printf("d %s/\n", cluster[j].item_name);
                }
                else if (in_ls.item_type == Itemtype_file) {
                    printf("- %s\n", cluster[j].item_name);
                }
            }
        }
    }
}


// TODO
static int list_indirect_links_lvl1(const struct inode* source) {
    size_t i;
    size_t items = 0;
    struct inode tmp;
    int32_t cluster[sb.count_dir_items];

//    for (i = 0; i < COUNT_INDIRECT_LINKS_1; ++i) {
//        if (links[i] != FREE_LINK) {
//            FS_SEEK_SET(sb.addr_data + links[i] * sb.cluster_size);
//            FS_READ(cluster, sizeof(int32_t), sb.count_links);
//            items = get_count_links(cluster);
//
//            list_direct_links(cluster);
//        }
//    }
}


// TODO
static int list_indirect_links_lvl2(const struct inode* source) {

}


int ls_(const char* path) {
    struct inode in_tmp;

    // no path given -- list actual directory
    if (strcmp(path, "") == 0) {
        memcpy(&in_tmp, &in_actual, sizeof(struct inode));
    }
    // else get last inode in path
    else {
        if (get_inode_by_path(&in_tmp, path) == RETURN_FAILURE) {
            in_tmp.item_type = Itemtype_free;
        }
    }

    switch (in_tmp.item_type) {
        case Itemtype_directory:
            list_direct_links(in_tmp.direct);
//            list_indirect_links_lvl1(&in_tmp); TODO
//            list_indirect_links_lvl2(&in_tmp); TODO
            break;

        case Itemtype_file:
            puts(path);
            break;

        default:
            set_myerrno(Err_item_not_exists);
    }

    return 0;
}