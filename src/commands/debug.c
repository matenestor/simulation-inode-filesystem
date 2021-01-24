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
			" block size: %d B\n"
			" block count: %d\n"
			" count of indirect links in block: %d\n"
			" count of directory item records in block: %d\n"
			" address of inodes bitmap: %d\n"
			" address of data bitmap: %d\n"
			" address of inodes: %d\n"
			" address of data: %d\n",
			sizeof(struct superblock),
			sb.signature,
			sb.volume_descriptor,
			sb.disk_size,
			sb.block_size,
			sb.block_count,
			sb.count_links,
			sb.count_dir_items,
			sb.addr_bm_inodes,
			sb.addr_bm_data,
			sb.addr_inodes,
			sb.addr_data);
}


static void debug_bitmaps() {
	size_t i, j;
	size_t total_fields = sb.block_count;
	size_t over_fields = total_fields % CACHE_SIZE;
	size_t addr_over_bm_in = sb.addr_bm_inodes + (total_fields - over_fields);
	size_t addr_over_bm_dt = sb.addr_bm_data + (total_fields - over_fields);
	size_t free_inodes_fields = 0;
	size_t free_block_fields = 0;
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
				++free_block_fields;
		}
	}

	puts("> BITMAPS ------------------------------");
	printf(" size of block: %d B\n"
			" free inodes: %d/%d\n"
			" free block: %d/%d\n",
			sb.block_size,
			free_inodes_fields, sb.block_count,
			free_block_fields, sb.block_count);
}


static void debug_inodes() {
	size_t i, j;
	// how many inodes can be read into 'CACHE_SIZE'
	size_t cache_capacity = CACHE_SIZE / sizeof(struct inode);
	size_t total_inodes = sb.block_count;
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
			inodes_free, sb.block_count,
			inodes_file, sb.block_count,
			inodes_dirc, sb.block_count);
}


static void debug_blocks() {
	size_t i, j, k;
	// how many data block can be read into 'CACHE_SIZE'
	size_t max_blocks = CACHE_SIZE / sb.block_size;
	// how many data block are outside of batches
	size_t loops = sb.block_count / max_blocks;
	size_t over_blocks = sb.block_count % max_blocks;
	size_t batch = loops > 0 ? max_blocks : over_blocks;

	size_t free_blocks = sb.block_count;
	char empty_block[sb.block_size];
	char blocks[batch * sb.block_size];

	memset(empty_block, '\0', sb.block_size);
	memset(blocks, '\0', max_blocks * sb.block_size);

	// read batches of data block
	fs_seek_set(sb.addr_data);

	for (i = 0; i <= loops; ++i) {
		batch = i < loops ? max_blocks : over_blocks;
		fs_read_char(blocks, sizeof(char), batch * sb.block_size);

		// check every block in cache
		for (j = 0; j < batch; ++j) {
			// compare data block with free block
			for (k = 0; k < sb.block_size; ++k) {
				// if block is not free, decrease available blocks
				if (blocks[j * sb.block_size + k] != empty_block[k]) {
					--free_blocks;
					break;
				}
			}
		}
	}

	puts("> BLOCKS -----------------------------");
	printf(" free block: %d/%d\n"
			" used block: %d/%d\n",
		   free_blocks, sb.block_count,
		   sb.block_count - free_blocks, sb.block_count);
}


int debug_(const char* detail) {
	debug_superblock();
	debug_bitmaps();
	debug_inodes();

	if (strcmp(detail, "all") == 0) {
		if (CACHE_SIZE >= sb.block_size) {
			debug_blocks();
		}
		else {
			printf("> BLOCK -----------------------------"
					" skip: cache size [%d kB] is smaller than size of block [%d kB]!",
					CACHE_SIZE/1024, sb.block_size / 1024);
		}
	}

	else if (strcmp(detail, "") != 0) {
		printf("debug: unknown argument [%s], use argument 'all' for debug also data block", detail);
	}

	puts("");

	return 0;
}
