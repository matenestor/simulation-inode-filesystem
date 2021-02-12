#include "stdlib.h"

#include "fs_api.h"
#include "fs_cache.h"

#include "errors.h"
#include "logger.h"

#define b2mb(b)	((b)/1024.0f/1024.0f)


int sim_df() {
	log_info("df");

	int ret = RETURN_FAILURE;
	size_t i;
	size_t free_inodes_fields = 0;
	size_t free_block_fields = 0;
	size_t inodes_free = 0;
	size_t inodes_file = 0;
	size_t inodes_dirc = 0;
	size_t used_space = 0;
	size_t total_space = sb.block_count * sb.block_size;
	bool* bitmap_inodes = malloc(sb.block_count);
	bool* bitmap_blocks = malloc(sb.block_count);
	// with bigger filesystem size (>4 GB) and bigger inodes,
	// this should be rewritten to iteration with caching the inodes
	struct inode* inodes = malloc(sb.block_count * sizeof(struct inode));

	if (inodes && bitmap_inodes && bitmap_blocks) {
		// read whole bitmaps and all inodes
		read_whole_bitmap_inodes(bitmap_inodes);
		read_whole_bitmap_data(bitmap_blocks);
		fs_read_inode(inodes, sb.block_count, 1);

		for (i = 0; i < sb.block_count; ++i) {
			// BITMAPS
			if (bitmap_inodes[i])
				free_inodes_fields++;
			if (bitmap_blocks[i])
				free_block_fields++;

			// INODES
			if (inodes[i].inode_type == Inode_type_free)
				inodes_free++;
			else if (inodes[i].inode_type == Inode_type_file)
				inodes_file++;
			else if (inodes[i].inode_type == Inode_type_dirc)
				inodes_dirc++;
		}

		used_space = (sb.block_count - free_block_fields) * sb.block_size;

		printf("GENERAL:\n"
			   " Size:\t%u MB\n Used:\t%.1f MB\n Avail:\t%.1f MB\n Use:\t%.1f %%\n",
			   sb.disk_size, b2mb(used_space), b2mb(total_space - used_space),
			   (float) used_space / total_space * 100.0f);

		printf("BITMAPS:\n"
			   " Free inodes: %ld/%d\n Free block:  %ld/%d\n",
			   free_inodes_fields, sb.block_count, free_block_fields, sb.block_count);

		printf("INODES:\n"
			   " Free inodes: %ld/%d\n"
			   " File inodes: %ld/%d\n"
			   " Dir  inodes: %ld/%d\n",
			   inodes_free, sb.block_count,
			   inodes_file, sb.block_count,
			   inodes_dirc, sb.block_count);

		ret = RETURN_SUCCESS;
	}
	else {
		set_myerrno(Err_malloc);
	}

	if (inodes)
		free(inodes);
	if (bitmap_inodes)
		free(bitmap_inodes);
	if (bitmap_blocks)
		free(bitmap_blocks);

	return ret;
}
