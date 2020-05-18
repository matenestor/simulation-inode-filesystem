#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "fs_operations.h"
#include "inc/fs_cache.h"
#include "inc/fs_prompt.h"
#include "inc/inode.h"
#include "inc/return_codes.h"
#include "utils.h"

#include "inc/logger_api.h"
#include "error.h"


// ================================================================================================
// START: Filesystem input/output functions.

void fs_seek_set(uint32_t offset) {
    if (offset > INT32_MAX) {
        offset -= INT32_MAX;
        fseek(filesystem, INT32_MAX, SEEK_SET);
        fseek(filesystem, offset, SEEK_CUR);
    }
    else {
        fseek(filesystem, offset, SEEK_SET);
    }
}

void fs_flush() {
    fflush(filesystem);
}

unsigned int fs_read_superblock(struct superblock* buffer, const size_t size, const size_t count) {
    return fread(buffer, size, count, filesystem);
}

unsigned int fs_read_inode(struct inode* buffer, const size_t size, const size_t count) {
    return fread(buffer, size, count, filesystem);
}

unsigned int fs_read_directory_item(struct directory_item* buffer, const size_t size, const size_t count) {
    return fread(buffer, size, count, filesystem);
}

unsigned int fs_read_int32t(int32_t* buffer, const size_t size, const size_t count) {
    return fread(buffer, size, count, filesystem);
}

unsigned int fs_read_bool(bool* buffer, const size_t size, const size_t count) {
    return fread(buffer, size, count, filesystem);
}

unsigned int fs_read_char(char* buffer, const size_t size, const size_t count) {
    return fread(buffer, size, count, filesystem);
}

unsigned int fs_write_superblock(const struct superblock* buffer, const size_t size, const size_t count) {
    return fwrite(buffer, size, count, filesystem);
}

unsigned int fs_write_inode(const struct inode* buffer, const size_t size, const size_t count) {
    return fwrite(buffer, size, count, filesystem);
}

unsigned int fs_write_directory_item(const struct directory_item* buffer, const size_t size, const size_t count) {
    return fwrite(buffer, size, count, filesystem);
}

unsigned int fs_write_int32t(const int32_t* buffer, const size_t size, const size_t count) {
    return fwrite(buffer, size, count, filesystem);
}

unsigned int fs_write_bool(const bool* buffer, const size_t size, const size_t count) {
    return fwrite(buffer, size, count, filesystem);
}

unsigned int fs_write_char(const char* buffer, const size_t size, const size_t count) {
    return fwrite(buffer, size, count, filesystem);
}

// END: Filesystem input/output functions.
// ================================================================================================


// ================================================================================================
// START: Filesystem init and close functions.

/******************************************************************************
 *
 *  Load a file with filesystem, if it exists. Read superblock, first inode and set 'is_formatted' to 'true'.
 *  If filesystem with given name does not exists, tell user about possible formatting.
 *
 */
int init_filesystem(const char* fsp, bool* is_formatted) {
    int ret = RETURN_FAILURE;

    log_info("Loading filesystem [%s].", fsp);

    // if filesystem exists, load it
    if (access(fsp, F_OK) == 0) {
        // filesystem is ready to be loaded
        if ((filesystem = fopen(fsp, "rb+")) != NULL) {
            // cache super block
            fs_read_superblock(&sb, sizeof(struct superblock), 1);
            // move to inodes location
            fs_seek_set(sb.addr_inodes);
            // cache root inode
            fs_read_inode(&in_actual, sizeof(struct inode), 1);

            puts("Filesystem loaded successfully.");
            *is_formatted = true;
            ret = RETURN_SUCCESS;

            log_info("Filesystem [%s] loaded.", fsp);
        }
        // filesystem is ready to be loaded, but there was an system error
        else {
            set_myerrno(Err_fs_not_loaded);
            *is_formatted = false;
        }
    }
        // else notify about possible formatting
    else {
        puts("No filesystem with this name found. You can format one with command 'format <size>'.");
        *is_formatted = false;
        ret = RETURN_SUCCESS;

        log_info("Filesystem [%s] not found.", fsp);
    }

    return ret;
}


/******************************************************************************
 *
 *  Close filesystem binary file.
 *
 */
void close_filesystem() {
    if (filesystem != NULL) {
        fclose(filesystem);
        log_info("Filesystem closed.");
    }
}

// END: Filesystem init and close functions.
// ================================================================================================


// ================================================================================================
// START: Filesystem inode search functions.

/******************************************************************************
 *
 *  Searches whole cluster with directory items. Function searches of either 'name' by 'id',
 *  or 'id' by 'name' in given inode 'source' in all links. What to search for is decided
 *  by given enum search_by 'key'. When variable, which is looked for, is found,
 *  function returns 'RETURN_SUCCESS'.
 *
 *  'key'   -- Determines if function searches for 'name', or 'id' in 'source'.
 *  'name'  -- When function searches for 'name', it is buffer, where result will be stored.
 *             If function searches for 'id', it is const string, which is searched with.
 *  'id'    -- When function searches for 'name', it is const int, which is searched with.
 *             If function searches for 'id', it is variable, where result will be stored.
 *  'links' -- Cluster with direct links pointing to clusters with directory items, which will be checked.
 *  'links_count' -- Count of links in 'links' cluster,
 *
 */
static int32_t search_cluster(const enum search_by key, char* name, int32_t* id, const int32_t* links, const size_t links_count) {
    int32_t ret = RETURN_FAILURE;
    size_t i, j, items;

    // array with directory items in cluster
    struct directory_item cluster[sb.count_dir_items];

    // check every link to cluster with directory items
    for (i = 0; i < links_count; ++i) {
        // other links are free
        if (links[i] == FREE_LINK) {
            continue;
        }

        // try to read maximum count in cluster -- only successful count of read elements is assigned
        fs_seek_set(sb.addr_data + links[i] * sb.cluster_size);
        fs_read_directory_item(cluster, sizeof(struct directory_item), sb.count_dir_items);
        items = get_count_dirs(cluster);

        // loop over existing directory items
        for (j = 0; j < items; ++j) {
            if (key == Id_by_name) {
                // inode of directory item with name 'name' found
                if (strcmp(name, cluster[j].item_name) == 0) {
                    *id = cluster[j].fk_id_inode;
                    ret = RETURN_SUCCESS;

                    log_info("Got item id [%d] by name [%s].", *id, name);

                    break;
                }
            }
            else if (key == Name_by_id) {
                // inode name with 'id' found
                if (cluster[j].fk_id_inode == *id) {
                    strncpy(name, cluster[j].item_name, STRLEN_ITEM_NAME - 1);
                    ret = RETURN_SUCCESS;

                    log_info("Got item name [%s] by id [%d].", name, *id);

                    break;
                }
            }
        }
    }

    return ret;
}


/******************************************************************************
 *
 *  Function checks every available link in given inode 'source'.
 *  Start with direct links, and continue with indirect links level 1 and level 2, if nothing was found.
 *  It reads every cluster with direct links and then call s function 'search_cluster(...)',
 *  where the cluster and variables are finally processed.
 *
 *  'key'    -- Determines if function checks for 'name', or 'id' in 'source'.
 *  'name'   -- When function checks for 'name', it is buffer, where result will be stored.
 *              If function checks for 'id', it is const string, which is checked with.
 *  'id'     -- When function checks for 'name', it is const int, which is checked with.
 *              If function checks for 'id', it is variable, where result will be stored.
 *  'source' -- Inode, which is being checked.
 *
 */
static int32_t search_links(const enum search_by key, char* name, int32_t* id, const struct inode* source) {
    int32_t ret = RETURN_FAILURE;
    size_t i, links;

    // cluster with direct links to be checked
    int32_t clstr_direct[sb.count_links];
    // cluster with indirect links to be checked
    int32_t clstr_indirect[sb.count_links];

    // first -- check direct links
    ret = search_cluster(key, name, id, source->direct, COUNT_DIRECT_LINKS);

    // second -- check indirect links of 1st level
    // note: if count of indirect links lvl 1 needs to be bigger than 1 in future, this `if` is needed to be in loop
    if (source->indirect1[0] != FREE_LINK && ret == RETURN_FAILURE) {
        // read whole cluster with 1st level indirect links to clusters with data
        fs_seek_set(sb.addr_data + source->indirect1[0] * sb.cluster_size);
        fs_read_int32t(clstr_direct, sizeof(int32_t), sb.count_links);
        links = get_count_links(clstr_direct);

        ret = search_cluster(key, name, id, clstr_direct, links);
    }

    // third -- check indirect links of 2nd level
    // note: if count of indirect links lvl 2 needs to be bigger than 1 in future, this `if` is needed to be in loop
    if (source->indirect2[0] != FREE_LINK && ret == RETURN_FAILURE) {
        // read whole cluster with 2nd level indirect links to clusters with 1st level indirect links
        fs_seek_set(sb.addr_data + source->indirect2[0] * sb.cluster_size);
        fs_read_int32t(clstr_indirect, sizeof(int32_t), sb.count_links);
        links = get_count_links(clstr_indirect);

        // loop over each 2nd level indirect link to get clusters with 1st level indirect links
        for (i = 0; i < links; ++i) {
            fs_seek_set(sb.addr_data + clstr_indirect[i] * sb.cluster_size);
            fs_read_int32t(clstr_direct, sizeof(int32_t), sb.count_links);
            links = get_count_links(clstr_direct);

            // name found
            if ((ret = search_cluster(key, name, id, clstr_direct, links)) != RETURN_FAILURE) {
                break;
            }
        }
    }

    if (key == Id_by_name && ret == RETURN_FAILURE) {
        set_myerrno(Err_fs_error);
        log_error("Unable to find name for id [%d]. Filesystem might be corrupted.", id);
    }

    return ret;
}


/******************************************************************************
 *
 *  Reads inode to 'in_dest' by given path. If path starts with "/",
 *  then function searches from root inode, else from actual inode, where user is.
 *  Inodes to final one are read and traversed folder by folder in path.
 *
 *  If inode was found, return 'RETURN_SUCCESS" in 'ret'.
 *
 */
int32_t get_inode_by_path(struct inode* in_dest, const char* path) {
    int32_t ret = RETURN_SUCCESS;
    int32_t id = 0;
    char* dir = NULL;
    char path_copy[strlen(path) + 1];

    strncpy(path_copy, path, strlen(path) + 1);

    // track path from root
    if (path_copy[0] == SEPARATOR[0]) {
        fs_seek_set(sb.addr_inodes);
        fs_read_inode(in_dest, sizeof(struct inode), 1);
    }
    // track path from actual directory
    else {
        fs_seek_set(sb.addr_inodes + in_actual.id_inode * sizeof(struct inode));
        fs_read_inode(in_dest, sizeof(struct inode), 1);
    }

    id = in_dest->id_inode;

    // parse path -- get first directory in given path
    dir = strtok(path_copy, SEPARATOR);

    // parse path -- get rest of the directories, if there are any
    while (dir != NULL) {
        // get inode of first directory in given path
        ret = search_links(Id_by_name, dir, &id, in_dest);

        // no item with parsed name found
        if (ret == RETURN_FAILURE) {
            set_myerrno(Err_item_not_exists);
            break;
        }
        // still some elements to parse -- continue
        else {
            fs_seek_set(sb.addr_inodes + id * sizeof(struct inode));
            fs_read_inode(in_dest, sizeof(struct inode), 1);
        }

        dir = strtok(NULL, SEPARATOR);
    }

    return ret;
}


/******************************************************************************
 *
 *  Get path from root to actual inode by going back from it to root over parents.
 *  Cache child inode and get if of its parent. Then cache parent inode
 *  and get name of its child. After retrieving name of child, append it to final
 *  path with preceding separator.
 *  If path would be too long for pwd, write at the beginning "..", instead of rest of path.
 *
 */
int32_t get_path_to_root(char* dest_path, const uint16_t length_new_path, bool* is_overflowed) {
    int32_t ret = RETURN_SUCCESS;
    int32_t size_used = length_new_path;
    uint16_t child_name_length = 0;
    int32_t id_child, id_parent = 0;
    struct inode in_tmp = {0};

    // buffer for names of directories on path to root
    char child_name[STRLEN_ITEM_NAME] = {0};
    // buffer for whole path to root
    char buffer[length_new_path];
    // move pointer to end of buffer
    char* p_buffer = buffer + length_new_path - 2;

    memset(buffer, '\0', length_new_path);

    // cache first inode id in path
    id_child = in_actual.id_inode;

    // while not in root inode
    while (id_child != 0) {
        // cache child inode
        fs_seek_set(sb.addr_inodes + id_child * sizeof(struct inode));
        fs_read_inode(&in_tmp, sizeof(struct inode), 1);

        // get id of parent inode
        search_links(Id_by_name, "..", &id_parent, &in_tmp);

        // cache parent inode
        fs_seek_set(sb.addr_inodes + id_parent * sizeof(struct inode));
        fs_read_inode(&in_tmp, sizeof(struct inode), 1);

        // get name of child inode
        if (search_links(Name_by_id, child_name, &id_child, &in_tmp) == RETURN_FAILURE) {
            ret = RETURN_FAILURE;
            break;
        }

        child_name_length = strlen(child_name);

        // write child name and separator to path
        if (size_used - (child_name_length + 1) > 1) {
            size_used -= (child_name_length + 1);

            // move before child name
            p_buffer -= child_name_length;
            // write name of child inode
            strncpy(p_buffer, child_name, child_name_length);
            --p_buffer;
            // write separator to path
            *p_buffer = SEPARATOR[0];

            // parent becomes a child
            id_child = in_tmp.id_inode;
        }
        // path to root is too long
        else {
            *is_overflowed = true;
            size_used = 0;
            --p_buffer;
            strncpy(p_buffer, "..", 2);
            break;
        }
    }

    if (ret == RETURN_SUCCESS) {
        // in case actual inode is root already, write root to path,
        // because it is possible that the while loop did not run
        if (size_used != 0) {
            *p_buffer = SEPARATOR[0];
        }

        // copy path to root to given buffer
        strncpy(dest_path, p_buffer, length_new_path);
    }

    return ret;
}


// END: Filesystem inode search functions.
// ================================================================================================

// ================================================================================================
// START: Filesystem bitmap managing functions.

static void bitmap_field_off(const int32_t address, const int32_t index) {
	bool f = false;
	fs_seek_set(address + index);
	fs_write_bool(&f, sizeof(bool), 1);

	fs_flush();
}


static void bitmap_field_on(const int32_t address, const int32_t index) {
	bool t = true;
	fs_seek_set(address + index);
	fs_write_bool(&t, sizeof(bool), 1);

	fs_flush();
}


/******************************************************************************
 *
 *  Finds first empty field in bitmap on given address. (Either inodes or data clusters bitmap).
 *  If no field is available, error is set.
 * 
 *  Returns index number of empty bitmap field, or 'RETURN_FAILURE'.
 *
 */
static int32_t get_empty_bitmap_field(const int32_t address) {
    size_t i, j;
    size_t index = RETURN_FAILURE;
    size_t loops = sb.cluster_count / CACHE_SIZE;
    size_t over_fields = sb.cluster_count % CACHE_SIZE;
    // count of field to be read
    size_t batch = loops > 0 ? CACHE_SIZE : over_fields;
    bool bitmap[batch];

    memset(bitmap, false, batch);

    for (i = 0; i <= loops; ++i) {
        batch = i < loops ? CACHE_SIZE : over_fields;

        // cache part of bitmap
        fs_seek_set(address + i * CACHE_SIZE);
        fs_read_bool(bitmap, sizeof(bool), batch);

        // check cached array for a free field
        for (j = 0; j < batch; ++j) {
            // check if field is free
            if (bitmap[j]) {
                // position of field in whole bitmap
                index = i * CACHE_SIZE + j;
                // turn off the index
                bitmap_field_off(address, index);

                break;
            }
        }

        // if free field was found, break
        if (index != RETURN_FAILURE) {
            log_info("Free cluster, type: [%s], index: [%d].", address == sb.addr_bm_inodes ? "inodes" : "data", index);

            break;
        }
    }

    // no more free inodes/data clusters
    if (index == RETURN_FAILURE) {
        if (address == sb.addr_bm_inodes) {
            set_myerrno(Err_inode_no_inodes);

            log_error("Out of inodes.");
        }
        else if (address == sb.addr_bm_data) {
            set_myerrno(Err_cluster_no_clusters);

            log_error("Out of data clusters.");
        }
        else {
            set_myerrno(Err_fs_error);

            log_error("Unknown address [%d]. Filesystem might be corrupted.", address);
        }
    }

    return index;
}

// END: Filesystem bitmap managing functions.
// ================================================================================================


// ================================================================================================
// START: Filesystem inode creation function.

/******************************************************************************
 *
 *  Create new inode for either directory or file. If inode is for directory,
 *  function also initializes '.' and '..' directories.
 *
 *  Returns index number of new inode, or 'RETURN_FAILURE'.
 *
 */
int32_t create_inode(struct inode* new_inode, const enum item type, const int32_t id_parent) {
    int32_t ret = RETURN_FAILURE;
    // id of new inode, which will be used
    int32_t id_free_inode = RETURN_FAILURE;
    // id of new cluster, which will be used for the inode
    int32_t id_free_cluster = RETURN_FAILURE;
    // directory item record, in case new inode is for directory
    struct directory_item new_dir_item[2] = {0};

    // find free field for inode, which is also id of the inode
    id_free_inode = get_empty_bitmap_field(sb.addr_bm_inodes);
    // find free field for link of the inode
    id_free_cluster = get_empty_bitmap_field(sb.addr_bm_data);

    // if there is free inode and data cluster for it, use it
    if (!(id_free_inode == RETURN_FAILURE || id_free_cluster == RETURN_FAILURE)) {
        // cache the free inode, in order to init it
        fs_seek_set(sb.addr_inodes + id_free_inode * sizeof(struct inode));
        fs_read_inode(new_inode, sizeof(struct inode), 1);

        // init new inode
        new_inode->item_type = type;
        new_inode->direct[0] = id_free_cluster;

        if (type == Itemtype_directory) {
            new_inode->file_size = sb.cluster_size;

            // create . directory
            strcpy(new_dir_item[0].item_name, ".");
            new_dir_item[0].fk_id_inode = id_free_inode;

            // create .. directory
            strcpy(new_dir_item[1].item_name, "..");
            new_dir_item[1].fk_id_inode = id_parent;

            fs_seek_set(sb.addr_data + id_free_cluster * sb.cluster_size);
            fs_write_directory_item(new_dir_item, sizeof(struct directory_item), 2);

            fs_flush();
        }

        // write new updated inode
        fs_seek_set(sb.addr_inodes + id_free_inode * sizeof(struct inode));
        fs_write_inode(new_inode, sizeof(struct inode), 1);

        fs_flush();

        ret = id_free_inode;

        log_info("New inode created, type: [%s], id: [%d].", type == Itemtype_directory ? "directory" : "file", id_free_inode);
    }
    else {
        log_error("Unable to create new inode.");
    }

    return ret;
}

// END: Filesystem inode creation function.
// ================================================================================================


// ================================================================================================
// START: Filesystem creation of links in inode.

/******************************************************************************
 *
 *  Initialize new link to empty data cluster.
 *
 *  Return index number of the cluster, or 'RETURN_FAILURE'.
 *
 */
static int32_t _init_link() {
    return get_empty_bitmap_field(sb.addr_bm_data);
}


/******************************************************************************
 *
 *  Initialize cluster with 'FREE_LINK's and first link to empty cluster.
 *
 *  Return index number of the cluster, or 'RETURN_FAILURE'.
 *
 */
static int32_t _init_cluster(int32_t id_cluster) {
    int32_t free_index = RETURN_FAILURE;
    int32_t cluster[sb.count_links];

    if ((free_index = _init_link()) != RETURN_FAILURE) {
        // init the cluster
        memset(cluster, FREE_LINK, sb.count_links);
        cluster[0] = free_index;

        // write initialized cluster
        fs_seek_set(sb.addr_data + id_cluster * sb.cluster_size);
        fs_write_int32t(cluster, sizeof(int32_t), sb.count_links);

        fs_flush();
    }

    return free_index;
}


/******************************************************************************
 *
 *  Clear cluster of every data inside.
 *
 */
static int32_t _clear_cluster(int32_t id_cluster) {
    char cluster[sb.cluster_size];

    // clear the cluster
    memset(cluster, '\0', sb.cluster_size);

    // write initialized cluster
    fs_seek_set(sb.addr_data + id_cluster * sb.cluster_size);
    fs_write_char(cluster, sizeof(char), sb.cluster_size);

    fs_flush();

    return 0;
}


/******************************************************************************
 *
 *  Create new direct link.
 *
 *  Returns index number of cluster where new link points to, or 'RETURN_FAILURE'.
 *
 */
static int32_t create_direct() {
    return _init_link();
}


/******************************************************************************
 *
 *  Create new indirect link level 1.
 *  
 *  Returns index number of cluster where new link points to, or 'RETURN_FAILURE'.
 *  
 */
static int32_t create_indirect_1(int32_t* link) {
    int ret = RETURN_FAILURE;
    int32_t id_clst_direct = RETURN_FAILURE;
    int32_t id_clst_data = RETURN_FAILURE;

    // init link to cluster of direct links
    if ((id_clst_direct = _init_link()) != RETURN_FAILURE) {
        // init cluster with direct links (return value is first link
        // with address to data cluster, or fail)
        if ((id_clst_data = _init_cluster(id_clst_direct)) != RETURN_FAILURE) {
            *link = id_clst_direct;
            ret = id_clst_data;
        }
        // cluster was not initialized == no more free clusters,
        // so turn on the cluster with direct links again
        else {
            bitmap_field_on(sb.addr_bm_data, id_clst_direct);
        }
    }

    return ret;
}


/******************************************************************************
 *
 *  Create new indirect link level 2.
 *  
 *  Returns index number of cluster where new link points to, or 'RETURN_FAILURE'.
 *  
 */
static int32_t create_indirect_2(int32_t* link) {
    int ret = RETURN_FAILURE;
    int32_t id_clst_indirect = RETURN_FAILURE;
    int32_t id_clst_direct = RETURN_FAILURE;
    int32_t id_clst_data = RETURN_FAILURE;

    // init cluster of indirect links level 1
    if ((id_clst_direct = create_indirect_1(&id_clst_indirect)) != RETURN_FAILURE) {
        // init cluster with direct links (return value is first link
        // with address to data cluster, or fail)
        if ((id_clst_data = _init_cluster(id_clst_direct)) != RETURN_FAILURE) {
            *link = id_clst_indirect;
            ret = id_clst_data;
        }
        // clusters were not initialized == no more free clusters, so clear cluster with indirect links
        // level 1 and turn on both clusters with indirect links level 1 and direct links again
        else {
            _clear_cluster(id_clst_indirect);
            bitmap_field_on(sb.addr_bm_data, id_clst_indirect);
            bitmap_field_on(sb.addr_bm_data, id_clst_direct);
        }
    }

    return ret;
}


// ================================================================================================


/******************************************************************************
 *
 *  Check if cluster is full of directory items.
 *
 *  Returns true if cluster is full, else false.
 *
 */
static bool is_cluster_full_dirs(const int32_t id_cluster) {
    bool is_full = false;
    size_t items = 0;
    struct directory_item cluster[sb.count_dir_items];

    fs_seek_set(sb.addr_data + id_cluster * sb.cluster_size);
    fs_read_directory_item(cluster, sizeof(struct directory_item), sb.count_dir_items);
    items = get_count_dirs(cluster);

    // cluster is full of directory records
    if (items == sb.count_dir_items) {
        is_full = true;
    }

    return is_full;
}


/******************************************************************************
 *
 *  Check if cluster is full of links.
 *
 *  Returns true if cluster is full, else false.
 *
 */
static bool is_cluster_full_links(const int32_t id_cluster) {
    bool is_full = false;
    size_t items = 0;
    int32_t cluster[sb.count_links];

    fs_seek_set(sb.addr_data + id_cluster * sb.cluster_size);
    fs_read_int32t(cluster, sizeof(int32_t), sb.count_links);
    items = get_count_links(cluster);

    // cluster is full of directory records
    if (items == sb.count_links) {
        is_full = true;
    }

    return is_full;
}


/******************************************************************************
 *
 *  Get first available link in given inode, or create new link in the inode.
 *
 *  Returns id of cluster where the link points to, or 'RETURN_FAILURE'.
 *
 */
int32_t get_link(struct inode* source) {
    int32_t id_free_cluster = RETURN_FAILURE;
    size_t i;
    // flag for changes in 'inode', about changes in clusters takes care functions themselves
    bool is_new_link = false;

    // check direct links for free one
    for (i = 0; i < COUNT_DIRECT_LINKS; ++i) {
        // if link in inode is free, create new link
        if (source->direct[i] == FREE_LINK) {
            if ((id_free_cluster = create_direct()) != RETURN_FAILURE) {
                source->direct[i] = id_free_cluster;
                is_new_link = true;
            }
            break;
        }
        else {
            if (!is_cluster_full_dirs(source->direct[i])) {
                id_free_cluster = source->direct[i];
                break;
            }
        }
    }

    // if new link was created, write the change to filesystem
    if (is_new_link) {
        fs_seek_set(sb.addr_inodes + source->id_inode * sizeof(struct inode));
        fs_write_inode(source, sizeof(struct inode), 1);

        fs_flush();
    }

	return id_free_cluster;
}

// END: Filesystem creation of links in inode.
// ================================================================================================
