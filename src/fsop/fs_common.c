#include <stdbool.h>
#include <unistd.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "inode.h"		// ROOT_ID

#include "logger.h"
#include "errors.h"


// filesystem file, which is being worked with
FILE* filesystem;

extern size_t fs_read_superblock(struct superblock* buffer);


int init_filesystem(const char* fsp, bool* is_formatted) {
	int ret = RETURN_FAILURE;
	log_info("Loading filesystem [%s].", fsp);

	// if filesystem exists, load it
	if (access(fsp, F_OK) == 0) {
		// filesystem is ready to be loaded
		if ((filesystem = fopen(fsp, "rb+")) != NULL) {
			fs_read_superblock(&sb);				// cache super block
			fs_read_inode(&in_actual, 1, ROOT_ID);	// cache root inode

			*is_formatted = true;
			puts("Filesystem loaded successfully.");

			log_info("Filesystem [%s] loaded.", fsp);
			ret = RETURN_SUCCESS;
		}
		// filesystem file couldn't be open
		else {
			*is_formatted = false;
			printf("Error while loading filesystem [%s].", fsp);
			my_perror("System error");
			reset_myerrno();
			log_critical("Filesystem [%s] invalid.", fsp);
		}
	}
	// else notify about possible formatting
	else {
		*is_formatted = false;
		puts("No filesystem with this name found. "
	   		"You can format one with command 'format <size>'.");

		log_info("Filesystem [%s] not found.", fsp);
		ret = RETURN_SUCCESS;
	}
	return ret;
}

void close_filesystem() {
	if (filesystem != NULL) {
		fclose(filesystem);
		log_info("Filesystem closed.");
	}
}

/*
 * Get count of data blocks in filesystem, which is needed,
 * in order to create new file inode.
 */
uint32_t get_count_data_blocks(const off_t file_size) {
	if (file_size % sb.block_size == 0)
		return file_size / sb.block_size;
	else
		return file_size / sb.block_size + 1;
}

/*
 * Check if there is enough space in filesystem for new file inode.
 * Function has to find out how many deep blocks will be needed to successfully create inode.
 * 	count_blocks -- how many blocks will data need (no deep blocks for indirect links)
 * 	count_empty_blocks -- how many blocks is available in filesystem
 */
bool is_enough_space(const uint32_t count_blocks, const uint32_t count_empty_blocks) {
	size_t i, j;
	uint32_t c_blocks = count_blocks;
	size_t addition = 0;	// additional deep blocks for indirect links

	for (i = 0; i < COUNT_DIRECT_LINKS && c_blocks > 0; ++i) {
		c_blocks -= 1;
	}
	for (i = 0; i < COUNT_INDIRECT_LINKS_1 && c_blocks > 0; ++i) {
		addition += 1;
		c_blocks -= sb.count_links;
	}
	for (i = 0; i < COUNT_INDIRECT_LINKS_2 && c_blocks > 0; ++i) {
		addition += 1;

		for (j = 0; j < sb.count_links && c_blocks > 0; ++j) {
			addition += 1;
			c_blocks -= sb.count_links;
		}
	}

	return (bool) (count_blocks + addition <= count_empty_blocks);
}
