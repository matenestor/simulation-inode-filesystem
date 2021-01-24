#include <stdio.h>
#include <string.h>

#include "fs_cache.h"
#include "inode.h"
#include "../fs_operations.h"


static void debug_superblock() {
    puts("> SUPERBLOCK ---------------------------");
    printf(" size of superblock: %d B\n"
           " signature: %s\n"
           " volume descriptor: %s\n"
           " disk size: %d MB\n"
           " cluster size: %d B\n"
           " cluster count: %d\n"
           " count of indirect links in cluster: %d\n"
           " count of directory item records in cluster: %d\n"
           " address of inodes bitmap: %d\n"
           " address of data bitmap: %d\n"
           " address of inodes: %d\n"
           " address of data: %d\n",
           sizeof(struct superblock),
           sb.signature,
           sb.volume_descriptor,
           sb.disk_size,
           sb.cluster_size,
           sb.cluster_count,
           sb.count_links,
           sb.count_dir_items,
           sb.addr_bm_inodes,
           sb.addr_bm_data,
           sb.addr_inodes,
           sb.addr_data);
}


static void debug_bitmaps() {
    size_t i, j;
    size_t total_fields = sb.cluster_count;
    size_t over_fields = total_fields % CACHE_SIZE;
    size_t addr_over_bm_in = sb.addr_bm_inodes + (total_fields - over_fields);
    size_t addr_over_bm_dt = sb.addr_bm_data + (total_fields - over_fields);
    size_t free_inodes_fields = 0;
    size_t free_clusters_fields = 0;
    bool bm_inodes[CACHE_SIZE];
    bool bm_data[CACHE_SIZE];
    size_t loops = total_fields / CACHE_SIZE;
    // size of field to be read
    size_t batch;

    // read batches of bitmap
    for (i = 0; i <= loops; ++i) {
        batch = i < loops ? CACHE_SIZE : over_fields;

        fs_seek_set(i < loops ? sb.addr_bm_inodes + i * CACHE_SIZE : addr_over_bm_in);
        fs_read_bool(bm_inodes, sizeof(bool), batch);

        fs_seek_set(i < loops ? sb.addr_bm_data + i * CACHE_SIZE : addr_over_bm_dt);
        fs_read_bool(bm_data, sizeof(bool), batch);

        for (j = 0; j < batch; ++j) {
            if (bm_inodes[j])
                ++free_inodes_fields;
            if (bm_data[j])
                ++free_clusters_fields;
        }
    }

    puts("> BITMAPS ------------------------------");
    printf(" size of cluster: %d B\n"
           " free inodes: %d/%d\n"
           " free clusters: %d/%d\n",
           sb.cluster_size,
           free_inodes_fields, sb.cluster_count,
           free_clusters_fields, sb.cluster_count);
}


static void debug_inodes() {
    size_t i, j;
    // how many inodes can be read into 'CACHE_SIZE'
    size_t cache_capacity = CACHE_SIZE / sizeof(struct inode);
    size_t total_inodes = sb.cluster_count;
    size_t over_inodes = total_inodes % cache_capacity;
    size_t inodes_free = 0;
    size_t inodes_file = 0;
    size_t inodes_dirc = 0;
    struct inode inodes[cache_capacity];
    size_t loops = total_inodes / cache_capacity;
    // count of inodes to be read, not their byte size!
    size_t batch;

    // read batches of inodes
    fs_seek_set(sb.addr_inodes);

    for (i = 0; i <= loops; ++i) {
        batch = i < loops ? cache_capacity : over_inodes;

        fs_read_inode(inodes, sizeof(struct inode), batch);

        for (j = 0; j < batch; ++j) {
            if (inodes[j].item_type == Itemtype_free)
                ++inodes_free;
            else if (inodes[j].item_type == Itemtype_file)
                ++inodes_file;
            else if (inodes[j].item_type == Itemtype_directory)
                ++inodes_dirc;
        }
    }

    puts("> INODES -------------------------------");
    printf(" size of inode: %d B\n"
           " count of direct links: %d\n"
           " count of indirect links level 1: %d\n"
           " count of indirect links level 2: %d\n"
           " free inodes: %d/%d\n"
           " file inodes: %d/%d\n"
           " directory inodes: %d/%d\n",
           sizeof(struct inode),
           COUNT_DIRECT_LINKS,
           COUNT_INDIRECT_LINKS_1,
           COUNT_INDIRECT_LINKS_2,
           inodes_free, sb.cluster_count,
           inodes_file, sb.cluster_count,
           inodes_dirc, sb.cluster_count);
}


static void debug_clusters() {
    size_t i, j, k;
    // how many data clusters can be read into 'CACHE_SIZE'
    size_t max_clusters = CACHE_SIZE / sb.cluster_size;
    // how many data clusters are outside of batches
    size_t loops = sb.cluster_count / max_clusters;
    size_t over_clusters = sb.cluster_count % max_clusters;
    size_t batch = loops > 0 ? max_clusters : over_clusters;

    size_t free_clusters = sb.cluster_count;
    char empty_cluster[sb.cluster_size];
    char clusters[batch * sb.cluster_size];

    memset(empty_cluster, '\0', sb.cluster_size);
    memset(clusters, '\0', max_clusters * sb.cluster_size);

    // read batches of data clusters
    fs_seek_set(sb.addr_data);

    for (i = 0; i <= loops; ++i) {
        batch = i < loops ? max_clusters : over_clusters;
        fs_read_char(clusters, sizeof(char), batch * sb.cluster_size);

        // check every cluster in cache
        for (j = 0; j < batch; ++j) {
            // compare data cluster with free cluster
            for (k = 0; k < sb.cluster_size; ++k) {
                // if cluster is not free, decrease available clusters
                if (clusters[j * sb.cluster_size + k] != empty_cluster[k]) {
                    --free_clusters;
                    break;
                }
            }
        }
    }

    puts("> CLUSTERS -----------------------------");
    printf(" free clusters: %d/%d\n"
           " used clusters: %d/%d\n",
           free_clusters, sb.cluster_count,
           sb.cluster_count - free_clusters, sb.cluster_count);
}


int debug_(const char* detail) {
    debug_superblock();
    debug_bitmaps();
    debug_inodes();

    if (strcmp(detail, "all") == 0) {
        if (CACHE_SIZE >= sb.cluster_size) {
            debug_clusters();
        }
        else {
            printf("> CLUSTERS -----------------------------"
                   " skip: cache size [%d kB] is smaller than size of cluster [%d kB]!",
                   CACHE_SIZE/1024, sb.cluster_size/1024);
        }
    }

    else if (strcmp(detail, "") != 0) {
        printf("debug: unknown argument [%s], use argument 'all' for debug also data clusters", detail);
    }

    puts("");

//    size_t i;
//    struct inode penis = {0};
//    int32_t cluster[256];
//    memset(cluster, -1, 256);
//
//    penis.id_inode = 1;
//    penis.item_type = Itemtype_file;
//
//    for (i = 0; i < COUNT_DIRECT_LINKS; ++i) {
//        penis.direct[i] = i+1;
//    }
//
//    penis.indirect1[0] = 6;
//    cluster[0] = 7; cluster[1] = 8; cluster[2] = 9;
//
//    fs_seek_set(sb.addr_data + 6 * sb.cluster_size);
//    fs_write_int32t(cluster, sizeof(int32_t), 256);
//
//    penis.indirect2[0] = 10;
//    cluster[0] = 11; cluster[1] = 12; cluster[2] = -1;
//
//    fs_seek_set(sb.addr_data + 10 * sb.cluster_size);
//    fs_write_int32t(cluster, sizeof(int32_t), 256);
//
//
//    cluster[0] = 13; cluster[1] = 14;
//    fs_seek_set(sb.addr_data + 11 * sb.cluster_size);
//    fs_write_int32t(cluster, sizeof(int32_t), 256);
//
//    cluster[0] = 15; cluster[1] = 16;
//    fs_seek_set(sb.addr_data + 12 * sb.cluster_size);
//    fs_write_int32t(cluster, sizeof(int32_t), 256);
//
//    fs_seek_set(sb.addr_inodes + penis.id_inode * sizeof(struct inode));
//    fs_write_inode(&penis, sizeof(struct inode), 1);
//    fs_flush();
//
//    destroy_inode(&penis);

    return 0;
}
