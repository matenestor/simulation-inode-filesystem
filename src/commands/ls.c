#include <stdint.h>
#include <string.h>

#include "fs_cache.h"
#include "fs_prompt.h"
#include "inode.h"
#include "../fs_operations.h"
#include "../utils.h"

#include "../../include/logger.h"
#include "../../include/errors.h"


static int list_direct_links(const int32_t* links) {
	size_t i, j;
	size_t items = 0;
	// cache inode for determination of file/directory
	struct inode in_ls;
	// block of item records to be printed
	struct directory_item block[sb.count_dir_items];

	for (i = 0; i < COUNT_DIRECT_LINKS; ++i) {
		// cache block with directory records pointed to by direct link
		if (links[i] != FREE_LINK) {
			fs_seek_set(sb.addr_data + links[i] * sb.block_size);
			fs_read_directory_item(block, sizeof(struct directory_item), sb.count_dir_items);
			items = get_count_dirs(block);

			// print names of records (directory + files)
			for (j = 0; j < items; ++j) {
				fs_seek_set(sb.addr_inodes + block[j].fk_id_inode * sizeof(struct inode));
				fs_read_inode(&in_ls, sizeof(struct inode), 1);

				if (in_ls.item_type == Itemtype_directory) {
					printf("d %s%s\n", block[j].item_name, SEPARATOR);
				}
				else if (in_ls.item_type == Itemtype_file) {
					printf("- %s\n", block[j].item_name);
				}
				// never should get here (but i got here during development, so from now,
				// i am covering all possible cases, even when they seem impossible)
				else {
					fprintf(stderr, "! %s [%d] leftover.\n", block[j].item_name, block[j].fk_id_inode);
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
	int32_t block[sb.count_dir_items];

//	for (i = 0; i < COUNT_INDIRECT_LINKS_1; ++i) {
//		if (links[i] != FREE_LINK) {
//			FS_SEEK_SET(sb.addr_data + links[i] * sb.block_size);
//			FS_READ(block, sizeof(int32_t), sb.count_links);
//			items = get_count_links(block);
//
//			list_direct_links(block);
//		}
//	}
}

// TODO
static int list_indirect_links_lvl2(const struct inode* source) {

}

/*
 * 	Get inode from given path and list items inside from direct links,
 * 	indirect links level 1 and indirect links level 2.
 */
int ls_(const char* path) {
	struct inode in_tmp;

	log_info("ls: [%s]", path);

	// no path given -- list actual directory
	if (strlen(path) == 0) {
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
//			list_indirect_links_lvl1(&in_tmp); TODO
//			list_indirect_links_lvl2(&in_tmp); TODO
			break;

		case Itemtype_file:
			puts(path);
			break;

		default:
			set_myerrno(Err_item_not_exists);
			log_warning("ls: unable to list [%s]", path);
	}

	return 0;
}
