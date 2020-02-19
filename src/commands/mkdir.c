#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../inc/fs_cache.h"
#include "../inc/inode.h"
#include "../inc/return_codes.h"
#include "../fs_operations.h"

#include "../error.h"


int mkdir_(char* path) {
    int ret = RETURN_FAILURE;
	int32_t id_inode_parent = -1;
	int32_t id_inode_child = -1;
	int32_t id_cluster = -1;
	size_t items = 0;
	struct directory_item dirs[sb.count_dir_items];
	struct directory_item new_dir;

	const size_t path_length = strlen(path);
	char path_copy[path_length];
	char name[STRLEN_ITEM_NAME] = {0};

	// copy path -- there are two tokenizing here
	strncpy(path_copy, path, path_length);

    // TODO
    //  - X! get destination inode
    //  - X check last nonempty link in inode
    //  - X check if in link's cluster is still space for another directory item
    //    - cREATE_INODE yes, mkdir -> get first free inode
    //    - X no, use another link in inode[1], mkdir

    // TODO [1]
    //  - X check if there is available link in inode (lvl1, lvl2, lvl3 link)
    //  - OPEN_NEW_LINK check if there is available data cluster

    if (path_length > 0) {
		// get name -- last element in path
		if (get_name(name, path_copy) != RETURN_FAILURE) {
			// copy path string again, because the copy was destroyed in get_name(...) by strtok
			strncpy(path_copy, path, path_length);

			// get inode, where the new directory should be created in
			id_inode_parent = get_inodeid_by_path(path_copy);

			if (id_inode_parent != RETURN_FAILURE) {
				// inode of last element in path gained
				FS_SEEK_SET(sb.addr_inodes + id_inode_parent);
				FS_READ(&in_distant, sizeof(struct inode), 1);

				// last nonempty link in inode, always will get something,
				// because 'id_inode_parent' in this place is not FREE_LINK
				id_cluster = get_last_link_value(&in_distant);

				// read cluster, where the link points
				FS_SEEK_SET(sb.addr_data + id_cluster);
				items = FS_READ(dirs, sizeof(struct directory_item), sb.count_dir_items);

				// the link points to full cluster -- open new link
				if (items == sb.count_dir_items) {
					id_cluster = open_new_link(&in_distant);
					items = 0;
				}

				// check if new link was opened
				// (it fails, when all links in indirect2 link are taken)
				if (id_cluster != RETURN_FAILURE) {
					// create inode for new directory record
					id_inode_child = create_inode(Item_directory, id_inode_parent);

					if (id_inode_child != RETURN_FAILURE) {
						// create a record of new directory
						strncpy(name, new_dir.item_name, STRLEN_ITEM_NAME);
						new_dir.fk_id_node = id_inode_child;
						memcpy(&dirs[items], &new_dir, sizeof(struct directory_item));

						// go back to start of cluster and write new directory record
						FS_SEEK_SET(sb.addr_data + id_cluster);
						FS_WRITE(dirs, sizeof(struct directory_item), items + 1);

						FS_FLUSH;

						ret = RETURN_SUCCESS;
					}
					else {
						my_perror("mkdir");
						reset_myerrno();
					}
				}
				else {
					my_perror("mkdir");
					reset_myerrno();
				}
			}
			else {
				my_perror("mkdir");
				reset_myerrno();
			}
		}
		else {
			my_perror("mkdir");
			reset_myerrno();
		}
    }
    else {
        set_myerrno(Arg_missing_operand);
    }

    return ret;
}
