#include <stdbool.h>
#include <string.h>

#include "fs_operations.h"
#include "inc/fs_cache.h"
#include "inc/inode.h"
#include "inc/return_codes.h"

#include "error.h"


bool is_free_inode() {
    bool free_inode = false;
    size_t i, j;
    bool bm_in[CACHE_SIZE] = {0};

    for (i = 0; i < sb.cluster_count; i += CACHE_SIZE) {
        // cache part of bitmap
        FS_SEEK_SET(sb.addr_bm_inodes);
        FS_READ(&bm_in, sizeof(bool), CACHE_SIZE);

        // check cached array for free inodes
        for (j = i; j < CACHE_SIZE; ++j) {
            // check if inode is free
            if (bm_in[j]) {
                free_inode = true;
                break;
            }
        }

        // if free inode was found, break
        if (free_inode)
            break;
    }


    return free_inode;
}


static int32_t get_item_by_name(const char* name, int32_t* links, const size_t size) {
    int32_t id = -1;
    size_t i, j, items;

    // maximum count of directory items in cluster
    const size_t count_dir_items = sb.cluster_size / sizeof(struct directory_item);
    // array with directory items in cluster
    struct directory_item cluster[count_dir_items];

    // check every link to cluster with directory items
    for (i = 0; i < size; ++i) {
        // other links are free
        if (links[i] == FREE_LINK) {
            break;
        }

        // try to read maximum count in cluster -- only successful count of read elements is assigned
        FS_SEEK_SET(sb.addr_data + links[i]);
        items = FS_READ(&cluster, sizeof(struct directory_item), count_dir_items);

        // loop over existing directory items
        for (j = 0; j < items; ++j) {
            if (strcmp(name, cluster[j].item_name) == 0) {
                id = cluster[j].fk_id_node;
                break;
            }
        }

        // inode of directory with name 'name' found
        if (id > -1)
            break;
    }

    return id;
}


static int32_t get_inodeid_by_inodename(const struct inode* source, const char* name) {
    int32_t id = -1;
    size_t i, items;

    // maximum count of (in)direct links in cluster
    const size_t count_links = sb.cluster_size / sizeof(int32_t);

    // array with links to be checked
    int32_t links[count_links];
    // array with indirect links in cluster
    int32_t indirect_links[count_links];

    // first -- check direct links
    links[0] = source->direct1;
    links[1] = source->direct2;
    links[2] = source->direct3;
    links[3] = source->direct4;
    links[4] = source->direct5;
    id = get_item_by_name(name, links, COUNT_DIRECT_LINKS);

    // second -- check indirect links of 1st level
    if (source->indirect1 != FREE_LINK && id == -1) {
        // read whole cluster with 1st level indirect links to clusters with data
        FS_SEEK_SET(sb.addr_data + source->indirect1);
        items = FS_READ(links, sizeof(int32_t), count_links);

        id = get_item_by_name(name, links, items);
    }

    // third -- check indirect links of 2nd level
    if (source->indirect2 != FREE_LINK && id == -1) {
        // read whole cluster with 2nd level indirect links to clusters with 1st level indirect links
        FS_SEEK_SET(sb.addr_data + source->indirect2);
        items = FS_READ(indirect_links, sizeof(int32_t), count_links);

        // loop over each 2nd level indirect link to get clusters with 1st level indirect links
        for (i = 0; i < items; ++i) {
            FS_SEEK_SET(sb.addr_data + indirect_links[i]);
            items = FS_READ(links, sizeof(int32_t), count_links);

            get_item_by_name(name, links, items);
        }
    }

    return id;
}


int32_t get_inodeid_by_path(char* path) {
    int32_t id = RETURN_FAILURE;
    char* dir = NULL;
    struct inode* p_source = NULL;

    // track path from root
    if (path[0] == SEPARATOR[0]) {
        FS_SEEK_SET(sb.addr_inodes);
        FS_READ(&in_distant, sizeof(struct inode), 1);
        p_source = &in_distant;
    }
    // track path from actual directory
    else {
        p_source = &in_actual;
    }

    // parse path -- get first directory in given path
    dir = strtok(path, SEPARATOR);

    // parse path -- get rest of the directories, if there are any
    while (dir != NULL) {
        // get inode of first directory in given path
        id = get_inodeid_by_inodename(p_source, dir);

        // get next element in path for further parsing and check of return value of get_inodeid_by_inodename(...)
        dir = strtok(NULL, SEPARATOR);

        // no item with parsed name found
        if (id == -1) {
            // element in path was last -- it is possible to make new directory
            if (dir == NULL) {
                id = p_source->id_node;
            }
            // element missing in path -- unable to make new directory
            else {
                id = RETURN_FAILURE;
                set_myerrno(Item_not_exists);
                break;
            }
        }
        else {
            // this was last element -- this dir already exists
            if (dir == NULL) {
                id = RETURN_FAILURE;
                set_myerrno(Dir_exists);
                break;
            }
            // still some elements to parse -- continue
            else {
                FS_SEEK_SET(sb.addr_inodes + id);
                FS_READ(&in_distant, sizeof(struct inode), 1);
                p_source = &in_distant; // TODO is this line redundant ?
            }
        }
    }

    return id;
}
