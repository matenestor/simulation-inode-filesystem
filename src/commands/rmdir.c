#include <stdint.h>
#include <string.h>

#include "fs_cache.h"
#include "inode.h"
#include "../fs_operations.h"
#include "../utils.h"

#include "../../include/logger.h"
#include "../../include/errors.h"


static int check_dot_dirs(const char* dir_name) {
    int ret = RETURN_FAILURE;

    if (strcmp(".", dir_name) == 0) {
        set_myerrno(Err_dir_arg_invalid);
    }
    else if (strcmp("..", dir_name) == 0) {
        set_myerrno(Err_dir_not_empty);
    }
    else {
        ret = RETURN_SUCCESS;
    }

    return ret;
}


static bool is_array_empty(const int32_t* array, const size_t size, const size_t begin) {
    bool ret = true;
    size_t i;

    for (i = begin; i < size; ++i) {
        if (array[i] != FREE_LINK) {
            ret = false;
            break;
        }
    }

    return ret;
}


static int is_dir_empty(const struct inode* in_target) {
    int ret = RETURN_FAILURE;

    if (is_array_empty(in_target->direct, COUNT_DIRECT_LINKS, 1)
            && is_array_empty(in_target->indirect1, COUNT_INDIRECT_LINKS_1, 0)
            && is_array_empty(in_target->indirect2, COUNT_INDIRECT_LINKS_2, 0)) {
        ret = RETURN_SUCCESS;
    }
    else {
        set_myerrno(Err_dir_not_empty);
    }

    return ret;
}


static int is_cluster_empty(const int32_t id_cluster) {
    int ret = RETURN_FAILURE;
    size_t items = 0;
    struct directory_item cluster[sb.count_dir_items];

    fs_seek_set(sb.addr_data + id_cluster * sb.cluster_size);
    fs_read_directory_item(cluster, sizeof(struct directory_item), sb.count_dir_items);
    items = get_count_dirs(cluster);

    // in directory are only "." and ".." directories
    if (items == 2) {
        ret = RETURN_SUCCESS;
    }
    else if (items > 2){
        set_myerrno(Err_dir_not_empty);
    }
    // never should get here, unless manually changing bytes in raw fs file
    else {
        set_myerrno(Err_fs_error);
    }

    return ret;
}


int rmdir_(const char* path) {
    int ret = RETURN_FAILURE;
    char dir_name[STRLEN_ITEM_NAME] = {0};
    struct inode in_rmdir = {0};

    if (strlen(path) > 0) {
        if (parse_name(dir_name, path, STRLEN_ITEM_NAME) != RETURN_FAILURE) {
            if (check_dot_dirs(dir_name) != RETURN_FAILURE) {
                if (get_inode_by_path(&in_rmdir, path) != RETURN_FAILURE) {
                    if (in_rmdir.item_type == Itemtype_directory) {
                        if (is_dir_empty(&in_rmdir) != RETURN_FAILURE) {
                            if (is_cluster_empty(in_rmdir.direct[0]) != RETURN_FAILURE) {
                                ret = destroy_inode(&in_rmdir);
                            }
                        }
                    }
                    else {
                        set_myerrno(Err_item_not_directory);
                    }
                }
            }
        }
    }
    else {
        set_myerrno(Err_arg_missing);
    }

    if (ret == RETURN_FAILURE) {
        log_warning("rmdir: unable to remove directory [%s]", path);
    }

    return ret;
}
