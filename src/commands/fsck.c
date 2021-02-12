#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "iteration_carry.h"

#include "errors.h"
#include "logger.h"

#define LOST_FOUND		"lost+found"
#define CHARS			"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
#define CHARS_AMOUNT	62	// amount of chars in CHARS [0-9A-Za-z]


static void generete_name(char* dest) {
	for (size_t i = 0; i < STRLEN_ITEM_NAME - 1; ++i) {
		dest[i] = CHARS[rand() % CHARS_AMOUNT];
	}
}

static int save_lost_inodes(struct inode* inode_lost_found, bool* inode_ids,
							const size_t ids_count, struct carry_fsck* carry_fsck) {
	size_t i;
	struct carry_dir_item carry_dir = {0};
	srand(time(NULL));

	for (i = 0; i < ids_count; ++i) {
		if (inode_ids[i]) {
			carry_dir.id = i + 1;
			generete_name(carry_dir.name);

			// save inode to lost+found directory
			if (add_to_parent(inode_lost_found, &carry_dir) == RETURN_FAILURE) {
				return RETURN_FAILURE;
			}

			inode_ids[i] = false;
			carry_fsck->active--;
		}
	}

	return RETURN_SUCCESS;
}

/*
 * Check filesystem consistency.
 */
int sim_fsck() {
	log_info("fsck");

	int active = 0;
	int total_lost = 0;
	struct inode inode_root = {0};
	struct inode inode_lost_found = {0};
	struct carry_fsck carry_fsck = {0};
	struct carry_dir_item carry_dir = {0};
	bool* inode_ids = NULL;

	if ((inode_ids = calloc(sb.block_count, sizeof(uint32_t))) == NULL) {
		set_myerrno(Err_malloc);
		goto fail;
	}
	// load all non-free inodes in filesystem (without root inode)
	if (load_inode_ids(inode_ids, sb.block_count, &active) == RETURN_FAILURE) {
		goto fail;
	}

	carry_fsck.inode_ids = inode_ids;
	carry_fsck.active = active;

	// load root inode and start iterating
	get_inode(&inode_root, "/");
	if (iterate_links(&inode_root, &carry_fsck, filter_correct_ids) == RETURN_SUCCESS) {
		// if 'filter_correct_ids()' returned positive value, it means error during mallocation
		printf("Filesystem status is NOT CHECKED. Unable to cache all inode ids.");
		goto fail;
	}

	// some inodes are still active == some inodes are lost
	if (carry_fsck.active > 0) {
		total_lost = carry_fsck.active;

		// load or create lost+found inode
		if (get_inode(&inode_lost_found, LOST_FOUND) != RETURN_FAILURE) {
			// got lost+found inode, good
		} else if (create_inode_directory(&inode_lost_found, ROOT_ID) != RETURN_FAILURE) {
			// create lost+found directory and add it to the root
			carry_dir.id = inode_lost_found.id_inode;
			strncpy(carry_dir.name, LOST_FOUND, STRLEN_ITEM_NAME);
			if (add_to_parent(&inode_root, &carry_dir) == RETURN_FAILURE) {
				free_inode_directory(&inode_lost_found);
				goto corrupted;
			}
		} else {
			goto corrupted;
		}

		// save lost inodes to lost+found directory
		if (save_lost_inodes(
				&inode_lost_found, inode_ids, sb.block_count, &carry_fsck) != RETURN_FAILURE) {
			printf("Filesystem status is REPAIRED. "
				   "Amount of saved inodes = %d.\n", total_lost);
		} else {
			printf("Filesystem status is CORRUPTED. Only some lost inodes saved. "
				   "Amount of lost inodes = %d.\n", carry_fsck.active);
			goto fail;
		}
	} else {
		puts("Filesystem status is OK. No lost inodes found.");
	}

	free(inode_ids);
	return RETURN_SUCCESS;

corrupted:
	printf("Filesystem status is CORRUPTED. Unable to create '"LOST_FOUND"' directory. "
		   "Amount of lost inodes = %d.\n", carry_fsck.active);
fail:
	if (inode_ids)
		free(inode_ids);
	log_warning("fsck: unable to check filesystem consistency");
	return RETURN_FAILURE;
}
