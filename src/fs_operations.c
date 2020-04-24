#include <stdbool.h>
#include <string.h>

#include "fs_operations.h"
#include "utils.h"
#include "inc/fs_cache.h"
#include "inc/inode.h"
#include "inc/return_codes.h"

#include "error.h"
#include "inc/logger_api.h"


/******************************************************************************
 *
 * 	Get directory or folder by its name in cluster of directory items.
 *
 *  Returns id of inode where item with given name is located, or 'RETURN_FAILURE'.
 *
 */
static int32_t get_item_by_name(const char* name, const int32_t* links, const size_t size) {
    int32_t id = RETURN_FAILURE;
    size_t i, j, items;

    // array with directory items in cluster
    struct directory_item cluster[sb.count_dir_items];

    // check every link to cluster with directory items
    for (i = 0; i < size; ++i) {
        // other links are free
        if (links[i] == FREE_LINK) {
            continue;
        }

        // try to read maximum count in cluster -- only successful count of read elements is assigned
        FS_SEEK_SET(sb.addr_data + links[i] * sb.count_dir_items * sizeof(struct directory_item));
        FS_READ(cluster, sizeof(struct directory_item), sb.count_dir_items);
        items = get_count_dirs(cluster);

        // loop over existing directory items
        for (j = 0; j < items; ++j) {
            if (strcmp(name, cluster[j].item_name) == 0) {
                id = cluster[j].fk_id_inode;
                break;
            }
        }

        // inode of directory item with name 'name' found
        if (id > RETURN_FAILURE) {
			log_info("Got item [%s] with id [%d].", name, id);

        	break;
        }
    }

    return id;
}


/******************************************************************************
 *
 *  Get inode id of directory or file by its name in given inode.
 *
 *  Returns id of the inode, or 'RETURN_FAILURE'.
 * 
 */
static int32_t get_inodeid_by_name(const struct inode* source, const char* name) {
    int32_t id = RETURN_FAILURE;
    size_t i, links;

    // cluster with direct links to be checked
    int32_t clstr_direct[sb.count_links];
    // cluster with indirect links to be checked
    int32_t clstr_indirect[sb.count_links];

    // first -- check direct links
    id = get_item_by_name(name, source->direct, COUNT_DIRECT_LINKS);

    // second -- check indirect links of 1st level
    // note: if count of indirect links lvl 1 needs to be bigger than 1 in future, this `if` is needed to be in loop
    if (source->indirect1[0] != FREE_LINK && id == RETURN_FAILURE) {
        // read whole cluster with 1st level indirect links to clusters with data
        FS_SEEK_SET(sb.addr_data + source->indirect1[0] * sb.count_links * sizeof(int32_t));
        FS_READ(clstr_direct, sizeof(int32_t), sb.count_links);
        links = get_count_links(clstr_direct);

        id = get_item_by_name(name, clstr_direct, links);
    }

    // third -- check indirect links of 2nd level
    // note: if count of indirect links lvl 2 needs to be bigger than 1 in future, this `if` is needed to be in loop
    if (source->indirect2[0] != FREE_LINK && id == RETURN_FAILURE) {
        // read whole cluster with 2nd level indirect links to clusters with 1st level indirect links
        FS_SEEK_SET(sb.addr_data + source->indirect2[0] * sb.count_links * sizeof(int32_t));
        FS_READ(clstr_indirect, sizeof(int32_t), sb.count_links);
        links = get_count_links(clstr_indirect);

        // loop over each 2nd level indirect link to get clusters with 1st level indirect links
        for (i = 0; i < links; ++i) {
            FS_SEEK_SET(sb.addr_data + clstr_indirect[i] * sb.count_links * sizeof(int32_t));
            FS_READ(clstr_direct, sizeof(int32_t), sb.count_links);
            links = get_count_links(clstr_direct);

            id = get_item_by_name(name, clstr_direct, links);

            // item found
            if (id != RETURN_FAILURE) {
                break;
            }
        }
    }

    return id;
}


/******************************************************************************
 *
 *  Reads inode to 'in_dest' by given path. If path starts with "/", 
 *  then function searches from root inode, else from actual inode, where user is.
 *  Inodes to final one are read and traversed folder by folder in path.
 *
 *  Returns id of the inode, if some was found, else 'RETURN_FAILURE'.
 *
 */
int32_t get_inode_by_path(struct inode* in_dest, const char* path) {
    int32_t id = RETURN_FAILURE;
    char* dir = NULL;
    char path_copy[strlen(path) + 1];

    strncpy(path_copy, path, strlen(path) + 1);

    // track path from root
    if (path_copy[0] == SEPARATOR[0]) {
        FS_SEEK_SET(sb.addr_inodes);
        FS_READ(in_dest, sizeof(struct inode), 1);
    }
    // track path from actual directory
    else {
        FS_SEEK_SET(sb.addr_inodes + sizeof(struct inode) * in_actual.id_inode);
        FS_READ(in_dest, sizeof(struct inode), 1);
    }

    id = in_dest->id_inode;

    // parse path -- get first directory in given path
    dir = strtok(path_copy, SEPARATOR);

    // parse path -- get rest of the directories, if there are any
    while (dir != NULL) {
        // get inode of first directory in given path
        id = get_inodeid_by_name(in_dest, dir);

        // no item with parsed name found
        if (id == RETURN_FAILURE) {
            set_myerrno(Err_item_not_exists);
            break;
        }
            // still some elements to parse -- continue
        else {
            FS_SEEK_SET(sb.addr_inodes + id * sizeof(struct inode));
            FS_READ(in_dest, sizeof(struct inode), 1);
        }

        dir = strtok(NULL, SEPARATOR);
    }

    return id;
}


static void bitmap_field_off(const int32_t address, const int32_t index) {
	bool f = false;
	FS_SEEK_SET(address + index);
	FS_WRITE(&f, sizeof(bool), 1);

	FS_FLUSH;
}


static void bitmap_field_on(const int32_t address, const int32_t index) {
	bool t = true;
	FS_SEEK_SET(address + index);
	FS_WRITE(&t, sizeof(bool), 1);

	FS_FLUSH;
}


/******************************************************************************
 *
 *  Finds first empty field in bitmap on given address. (Either inodes or data clusters bitmap).
 *
 *  Returns index number of empty bitmap field, or 'RETURN_FAILURE'.
 *
 */
static int32_t get_empty_bitmap_field(const int32_t address) {
	size_t i, j, items;
	size_t index = RETURN_FAILURE;
	bool bitmap[CACHE_SIZE] = {0};

	for (i = 0; i < sb.cluster_count; i += CACHE_SIZE) {
		// cache part of bitmap
		FS_SEEK_SET(address + i);
		items = FS_READ(bitmap, sizeof(bool), CACHE_SIZE);

		// check cached array for a free field
		for (j = 0; j < items; ++j) {
			// check if field is free
			if (bitmap[j]) {
				// position of field in whole bitmap
				index = i * CACHE_SIZE + j;

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
	struct directory_item new_dir_item[2];

	// find free field for inode, which is also id of the inode
	id_free_inode = get_empty_bitmap_field(sb.addr_bm_inodes);
	// find free field for link of the inode
	id_free_cluster = get_empty_bitmap_field(sb.addr_bm_data);

	// if there is free inode and data cluster for it, use it
	if (!(id_free_inode == RETURN_FAILURE || id_free_cluster == RETURN_FAILURE)) {
		bitmap_field_off(sb.addr_bm_inodes, id_free_inode);
		bitmap_field_off(sb.addr_bm_data, id_free_cluster);

		// cache the free inode, in order to init it
		FS_SEEK_SET(sb.addr_inodes + id_free_inode * sizeof(struct inode));
		FS_READ(new_inode, sizeof(struct inode), 1);

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

            FS_SEEK_SET(sb.addr_data + id_free_cluster * sb.count_dir_items * sizeof(struct directory_item));
            FS_WRITE(&new_dir_item, sizeof(struct directory_item), 2);

			FS_FLUSH;
		}

		// write new updated inode
		FS_SEEK_SET(sb.addr_inodes + id_free_inode * sb.count_dir_items * sizeof(struct directory_item));
		FS_WRITE(new_inode, sizeof(struct inode), 1);

		FS_FLUSH;

		ret = id_free_inode;

		log_info("New inode created, type: [%s], id: [%d].", type == Itemtype_directory ? "directory" : "file", id_free_inode);
	}
	else {
		log_error("Unable to create new inode.");
	}

	return ret;
}


// ================================================================================================


/******************************************************************************
 *
 *  Initialize new link to empty data cluster.
 *
 *  Return index number of the cluster, or 'RETURN_FAILURE'.
 *
 */
static int32_t _init_link() {
	int32_t free_index = RETURN_FAILURE;

	free_index = get_empty_bitmap_field(sb.addr_bm_data);

	if (free_index != RETURN_FAILURE) {
		bitmap_field_off(sb.addr_bm_data, free_index);
	}

	return free_index;
}


/******************************************************************************
 *
 *  Initialize cluster with 'FREE_LINK's and first link to empty cluster.
 *
 *  Return index number of the cluster, or 'RETURN_FAILURE'.
 *
 */
static int32_t _init_cluster(int32_t address) {
	int32_t free_index = RETURN_FAILURE;
	int32_t cluster[sb.count_links];

	if ((free_index = _init_link()) != RETURN_FAILURE) {
		// init the cluster
		memset(cluster, FREE_LINK, sb.count_links);
		cluster[0] = free_index;

        // write initialized cluster
        FS_SEEK_SET(sb.addr_data + address * sb.count_links * sizeof(int32_t));
        FS_WRITE(cluster, sizeof(int32_t), sb.count_links);

        FS_FLUSH;
	}

	return free_index;
}


/******************************************************************************
 *
 *  Clear cluster of every data inside.
 *
 */
static int32_t _clear_cluster(int32_t address) {
    int32_t cluster[sb.count_links];

    // clear the cluster
    memset(cluster, '\0', sb.count_links);

    // write initialized cluster
    FS_SEEK_SET(sb.addr_data + address * sb.cluster_size * sizeof(char));
    FS_WRITE(cluster, sizeof(char), sb.cluster_size);

    FS_FLUSH;

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
	int32_t addr_cluster = RETURN_FAILURE;
	int32_t addr_data = RETURN_FAILURE;

	// init link to cluster of direct links
	if ((addr_cluster = _init_link()) != RETURN_FAILURE) {
		// init cluster with direct links (return value is first link
		// with address to data cluster, or fail)
		if ((addr_data = _init_cluster(addr_cluster)) != RETURN_FAILURE) {
            *link = addr_cluster;
			ret = addr_data;
		}
		// cluster was not initialized == no more free clusters,
		// so turn on the cluster with direct links again
		else {
			bitmap_field_on(sb.addr_bm_data, addr_cluster);
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
	int32_t addr_cluster_indir1 = RETURN_FAILURE;
	int32_t addr_cluster_dir = RETURN_FAILURE;
	int32_t addr_data = RETURN_FAILURE;

    // init cluster of indirect links level 1
    if ((addr_cluster_dir = create_indirect_1(&addr_cluster_indir1)) != RETURN_FAILURE) {
        // init cluster with direct links (return value is first link
        // with address to data cluster, or fail)
        if ((addr_data = _init_cluster(addr_cluster_dir)) != RETURN_FAILURE) {
            *link = addr_cluster_indir1;
            ret = addr_data;
        }
        // clusters were not initialized == no more free clusters, so clear cluster with indirect links
        // level 1 and turn on both clusters with indirect links level 1 and direct links again
        else {
            _clear_cluster(addr_cluster_indir1);
            bitmap_field_on(sb.addr_bm_data, addr_cluster_indir1);
            bitmap_field_on(sb.addr_bm_data, addr_cluster_dir);
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

    FS_SEEK_SET(sb.addr_data + id_cluster * sb.count_dir_items * sizeof(struct directory_item));
    FS_READ(cluster, sizeof(struct directory_item), sb.count_dir_items);
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

    FS_SEEK_SET(sb.addr_data + id_cluster * sb.count_links * sizeof(int32_t));
    FS_READ(cluster, sizeof(int32_t), sb.count_links);
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
        FS_SEEK_SET(sb.addr_inodes + source->id_inode * sizeof(struct inode));
        FS_WRITE(source, sizeof(struct inode), 1);

        FS_FLUSH;
    }

	return id_free_cluster;
}
