#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "iteration_carry.h"

#include "errors.h"
#include "logger.h"

// DISCLAIMER: these functions will stay only in this file!

#define CHANCE	20		// % chance to get item deleted


ITERABLE(corrupt_items) {
	size_t i, j;
	struct directory_item block[sb.count_dir_items];

	for (i = 0; i < links_count; ++i) {
		if (links[i] == FREE_LINK)
			continue;

		fs_read_directory_item(block, sb.count_dir_items, links[i]);

		for (j = 0; j < sb.count_dir_items; ++j) {
			// delete other than dot and empty directories
			if (strcmp(block[j].item_name, "") == 0
					|| strcmp(block[j].item_name, ".") == 0
					|| strcmp(block[j].item_name, "..") == 0)
				continue;

			if (rand() % (100 / CHANCE) == 0) {
				printf("deleting: [id: %d] [name: %s]\n", block[j].id_inode, block[j].item_name);
				block[j].id_inode = FREE_LINK;
				strncpy(block[j].item_name, "", STRLEN_ITEM_NAME);
				fs_write_directory_item(block, sb.count_dir_items, links[i]);
				return true;
			}
		}
	}
	return false;
}

/*
 * Iterates over all directory inodes in filesystem
 * and randomly deletes item records in blocks.
 */
int corrupt_inodes() {
	size_t i;
	// with bigger filesystem size (>4 GB) and bigger inodes,
	// this should be rewritten to iteration with caching the inodes
	struct inode* inode_corrupt = malloc(sb.block_count * sizeof(struct inode));

	if (inode_corrupt) {
		// read all inodes in filesystem
		fs_read_inode(inode_corrupt, sb.block_count, 1);

		for (i = 0; i < sb.block_count; ++i) {
			if (inode_corrupt[i].inode_type != Inode_type_dirc)
				continue;

			iterate_links(&inode_corrupt[i], NULL, corrupt_items);
		}

		free(inode_corrupt);
	}
}


int sim_corrupt() {
	log_info("corrupt");

	srand(time(NULL));

	corrupt_inodes();

	return RETURN_SUCCESS;
}
