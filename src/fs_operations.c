#include <stdbool.h>
#include <string.h>

#include "fs_operations.h"
#include "inc/fs_cache.h"
#include "inc/inode.h"
#include "inc/return_codes.h"

#include "error.h"


static int32_t get_item_by_name(const char* name, int32_t* links, const size_t size) {
    int32_t id = -1;
    size_t i, j, items;

    // array with directory items in cluster
    struct directory_item cluster[sb.count_dir_items];

    // check every link to cluster with directory items
    for (i = 0; i < size; ++i) {
        // other links are free
        if (links[i] == FREE_LINK) {
            break;
        }

        // try to read maximum count in cluster -- only successful count of read elements is assigned
        FS_SEEK_SET(sb.addr_data + links[i]);
        items = FS_READ(&cluster, sizeof(struct directory_item), sb.count_dir_items);

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

    // array with links to be checked
    int32_t links[sb.count_links];
    // array with indirect links in cluster
    int32_t indirect_links[sb.count_links];

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
        items = FS_READ(links, sizeof(int32_t), sb.count_links);

        id = get_item_by_name(name, links, items);
    }

    // third -- check indirect links of 2nd level
    if (source->indirect2 != FREE_LINK && id == -1) {
        // read whole cluster with 2nd level indirect links to clusters with 1st level indirect links
        FS_SEEK_SET(sb.addr_data + source->indirect2);
        items = FS_READ(indirect_links, sizeof(int32_t), sb.count_links);

        // loop over each 2nd level indirect link to get clusters with 1st level indirect links
        for (i = 0; i < items; ++i) {
            FS_SEEK_SET(sb.addr_data + indirect_links[i]);
            items = FS_READ(links, sizeof(int32_t), sb.count_links);

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


int get_name(char* name, char* path) {
	int ret = RETURN_FAILURE;
	char* token = NULL;

	// parse whole path, until last element, which is new name
	for (token = strtok(path, SEPARATOR); token != NULL; token = strtok(NULL, SEPARATOR)) {
		strncpy(name, path, STRLEN_ITEM_NAME);
	}

	// check if given name is not too long
	if (name[STRLEN_ITEM_NAME - 1] != '\0') {
		ret = RETURN_FAILURE;
		set_myerrno(Item_name_long);
	}

	return ret;
}


int32_t get_last_link_value(const struct inode* source) {
	size_t i;
	int32_t value = FREE_LINK;
	// links in REVERSED order
	int32_t link_values[] = {source->indirect2, source->indirect1, source->direct5, source->direct4, source->direct3, source->direct2, source->direct1};

	// check each link, if it point to some cluster
	for (i = 0; i < LEN(link_values); ++i) {
		// if yes, get its value and break
		if (link_values[i] != FREE_LINK) {
			value = link_values[i];
			break;
		}
	}

	return value;
}


static int32_t get_bitmap_empty_field(const int32_t address) {
	size_t i, j, items;
	size_t index = 0;
	bool bitmap[CACHE_SIZE] = {0};

	for (i = 0; i < sb.cluster_count; i += CACHE_SIZE) {
		// cache part of bitmap
		FS_SEEK_SET(address + i);
		items = FS_READ(bitmap, sizeof(bool), CACHE_SIZE);

		// check cached array for a free field
		for (j = 0; j < items; ++j) {
			// check if field is free
			if (bitmap[j]) {
				bitmap[j] = false; // TODO fwrite this change
				index = i * CACHE_SIZE + j;
				break;
			}
		}

		// if free field was found, break
		if (index != 0)
			break;
	}

	// if index is 0 even after whole loop, it means,
	// that there are no more free inodes/data clusters
	if (index == 0) {
		if (address == sb.addr_bm_inodes) {
			set_myerrno(Inode_no_inodes);
		}
		else if (address == sb.addr_bm_data) {
			set_myerrno(Cluster_no_clusters);
		}
	}

	return index;
}


int32_t open_new_link(struct inode* source) {
	int ret = RETURN_FAILURE;
	size_t i;
	int32_t free_cluster_index = 0;
	// links in SEQUENTIAL order
	int32_t link_values[] = {source->direct1, source->direct2, source->direct3, source->direct4, source->direct5, source->indirect1, source->indirect2};

	// check each link, in order to find first free
	for (i = 0; i < LEN(link_values); ++i) {
		if (link_values[i] == FREE_LINK) {
			// TODO init link with data cluster
			//  - first empty block from bitmap

			// find free field
			free_cluster_index = get_bitmap_empty_field(sb.addr_bm_data);

			// if there is free field, use it
			if (free_cluster_index != 0) {
				link_values[i] = free_cluster_index; // TODO fwrite this change
				ret = RETURN_SUCCESS;
			}

			break;
		}
	}

	return ret;
}


int32_t create_inode(enum item type, int32_t parent) {
	int ret = RETURN_FAILURE;
	int32_t free_inode_index = 0;
	struct inode new_inode = {0};

	// TODO
	// find free field
	free_inode_index = get_bitmap_empty_field(sb.addr_bm_inodes);

	// if there is free field, use it
	if (free_inode_index != 0) {
		FS_SEEK_SET(sb.addr_inodes + free_inode_index);
		FS_READ(&new_inode, sizeof(struct inode), 1);

		new_inode.item_type = type;

		// TODO
		//  if type == directory
		//  - create data block for inode
		//  - create . (new_inode.id_inode) and .. (parent) directories in new inode
		//  else (it is file)
		//  - just take data block for data

		// write new updated inode
		FS_SEEK_SET(sb.addr_inodes + free_inode_index);
		FS_WRITE(&new_inode, sizeof(struct inode), 1);

		ret = RETURN_SUCCESS;
	}

	return ret;
}

