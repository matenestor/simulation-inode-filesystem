#include <stdbool.h>

#include "inc/inode.h"
#include "inc/fs_cache.h"
#include "fs_operations.h"


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
