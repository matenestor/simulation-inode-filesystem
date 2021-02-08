#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "inode.h"

#define CACHE_SIZE	131072

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

size_t fs_read_bool(bool* buffer, size_t count);

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

		fs_read_bool(bm_inodes, i < loops ? sb.addr_bm_inodes + i * CACHE_SIZE : addr_over_bm_in);
		fs_read_bool(bm_data,   i < loops ? sb.addr_bm_data   + i * CACHE_SIZE : addr_over_bm_dt);

		for (j = 0; j < batch; ++j) {
			if (bm_inodes[j])
				++free_inodes_fields;
			if (bm_data[j])
				++free_block_fields;
		}
	}

	puts("> BITMAPS ------------------------------");
	printf(" size of block: %d B\n"
			" free inodes: %ld/%d\n"
			" free block: %ld/%d\n",
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
	for (i = 0; i <= loops; ++i) {
		batch = i < loops ? cache_capacity : over_inodes;

		fs_read_inode(inodes, batch, i * cache_capacity);

		for (j = 0; j < batch; ++j) {
			if (inodes[j].inode_type == Inode_type_free)
				++inodes_free;
			else if (inodes[j].inode_type == Inode_type_file)
				++inodes_file;
			else if (inodes[j].inode_type == Inode_type_dirc)
				++inodes_dirc;
		}
	}

	puts("> INODES -------------------------------");
	printf(" size of inode: %ld B\n"
			" count of direct links: %d\n"
			" count of indirect links level 1: %d\n"
			" count of indirect links level 2: %d\n"
			" free inodes: %ld/%d\n"
			" file inodes: %ld/%d\n"
			" directory inodes: %ld/%d\n",
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
	for (i = 0; i <= loops; ++i) {
		batch = i < loops ? max_blocks : over_blocks;
		fs_read_data(blocks, batch * sb.block_size, i * max_blocks);

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
	printf(" free block: %ld/%d\n"
			" used block: %ld/%d\n",
		   free_blocks, sb.block_count,
		   sb.block_count - free_blocks, sb.block_count);
}

static int print_links(const char* type, const uint32_t* links, const size_t links_count) {
	printf("%s", type);
	for (size_t i = 0; i < links_count; ++i) {
		printf(" %d", links[i]);
	}
	puts("");
	return 0;
}

void inode(const uint32_t id) {
	struct inode inode_item;
	fs_read_inode(&inode_item, 1, id);
	printf(" type: %s\n", inode_item.inode_type == Inode_type_dirc ? "directory"
						  : inode_item.inode_type == Inode_type_file ? "file" : "free");
	printf(" size: %d B = %.2f kB = %.2f MB\n",
		   inode_item.file_size,
		   (double) inode_item.file_size / 1024,
		   (double) inode_item.file_size / 1024 / 1024);
	printf(" inode id: %d\n", inode_item.id_inode);
	print_links(" direct links:", inode_item.direct, COUNT_DIRECT_LINKS);
	print_links(" indrct links lvl 1:", inode_item.indirect_1, COUNT_INDIRECT_LINKS_1);
	print_links(" indrct links lvl 2:", inode_item.indirect_2, COUNT_INDIRECT_LINKS_2);
}

void block(const uint32_t id) {
	struct directory_item block[sb.count_dir_items];
	fs_read_directory_item(block, sb.count_dir_items, id);
	for (size_t i = 0; i < sb.count_dir_items; ++i) {
		if (strcmp(block[i].item_name, "") != 0) {
			printf("%d\t%s\n", block[i].id_inode, block[i].item_name);
		}
	}
}

int sim_debug(const char* flag, const char* str_id) {
	long id;

	if (strcmp(flag, "all") == 0) {
		debug_superblock();
		debug_bitmaps();
		debug_inodes();
		debug_blocks();
	} else {
		id = strtol(str_id, NULL, 10);
		if (strcmp(flag, "i") == 0) {
			inode(id);
		}
		else if (strcmp(flag, "b") == 0) {
			block(id);
		}
	}

	return 0;
}
