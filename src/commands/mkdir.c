#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "../fs_operations.h"
#include "../utils.h"
#include "../inc/fs_cache.h"
#include "../inc/inode.h"
#include "../inc/return_codes.h"

#include "../inc/logger_api.h"
#include "../error.h"


/******************************************************************************
 *
 * 	Check if making new directory is possible by reading parent of the directory
 * 	and then trying to read inode with name of the directory.
 *
 */
static bool is_mkdir_possible(const char* path, const char* path_parent) {
    bool is_creatable = false;
    struct inode in_tmp = {0};

    // path to destination inode exists
    if (get_inode_by_path(&in_tmp, path_parent) != RETURN_FAILURE) {
        // no directory with same name as new being created exists
        if (get_inode_by_path(&in_tmp, path) == RETURN_FAILURE) {
            is_creatable = true;
        }
        else {
            set_myerrno(Err_dir_exists);
        }
    }

    return is_creatable;
}


/******************************************************************************
 *
 * 	Makes new directory. Check if path was given, parse name from path,
 * 	check if directory creation is possible, get inode where the directory will be created,
 * 	get free link in the inode, create new inode for the directory.
 * 	If everything above is successful, initialize new directory and make record of it
 * 	in parent inode and data cluster.
 *
 */
int mkdir_(const char* path) {
    int ret = RETURN_FAILURE;

    log_info("mkdir: creating [%s]", path);

    // count of records in cluster read
    size_t items = 0;
    // name of new directory
    char dir_name[STRLEN_ITEM_NAME] = {0};
    // path to parent of new directory, if any
    char path_parent[strlen(path) + 1];
    // id of cluster, where record of new directory will be stored
    int32_t id_cluster = RETURN_FAILURE;
    // parent of directory being created
    struct inode in_parent = {0};
    // inode of new directory
    struct inode in_new_dir = {0};
    // struct of new directory, which will be put to cluster
    struct directory_item new_dir = {0};
    // cluster of directory records, where the new record will be stored
    struct directory_item dirs[sb.count_dir_items];

    // separate path to parent directory from new directory name
    parse_parent_path(path_parent, path);

    if (strlen(path) > 0) {
        // get name -- last element in path
        if (parse_name(dir_name, path, STRLEN_ITEM_NAME) != RETURN_FAILURE) {
            // check if it is possible to make new directory
            if (is_mkdir_possible(path, path_parent)) {
                // get parent inode, where new directory should be created in
                if (get_inode_by_path(&in_parent, path_parent) != RETURN_FAILURE) {
                    // get link to cluster in parent, where new directory will be written to
                    if ((id_cluster = get_link(&in_parent)) != RETURN_FAILURE) {
                        // create inode for new directory record
                        if (create_inode(&in_new_dir, Itemtype_directory, in_parent.id_inode) != RETURN_FAILURE) {
                            // cache cluster where the record of new directory will be stored
                            fs_seek_set(sb.addr_data + id_cluster * sb.cluster_size);
                            fs_read_directory_item(dirs, sizeof(struct directory_item), sb.count_dir_items);
                            items = get_count_dirs(dirs);

                            // init new directory
                            strncpy(new_dir.item_name, dir_name, strlen(dir_name) + 1);
                            new_dir.fk_id_inode = in_new_dir.id_inode;

                            // insert new dir to the cluster
                            dirs[items] = new_dir;

                            // write updated cluster
                            fs_seek_set(sb.addr_data + id_cluster * sb.cluster_size);
                            fs_write_directory_item(dirs, sizeof(struct directory_item), items + 1);

                            fs_flush();

                            ret = RETURN_SUCCESS;
                        }
                    }
                }
            }
        }
    }
    else {
        set_myerrno(Err_arg_missing_operand);
        log_warning("mkdir: unable to make directory [%s]", path);
    }

    return ret;
}
