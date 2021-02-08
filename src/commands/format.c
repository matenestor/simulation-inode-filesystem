#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "inode.h"

#include "logger.h"
#include "errors.h"

// --- FILESYSTEM CONFIG
#define FS_SIZE_MAX				4095	// maximal filesystem size in MB (because using 32b ints)
#define FS_BLOCK_SIZE			1024	// filesystem block size in bytes B
#define PERCENTAGE				0.95	// percentage of space for data in filesystem
#define CACHE_SIZE				131072	// cache size for fwriting and freading (128 kB) TODO 1 MB with malloc?
// --- FILESYSTEM CONFIG

#define LOG_DATETIME_LENGTH_	25
#define mb2b(mb)				((mb)*1024UL*1024UL)
#define isnegnum(cha)			(cha[0] == '-')
#define isinrange(n)			((n) > 0 && (n) <= FS_SIZE_MAX)

extern FILE* filesystem;

extern size_t fs_write_superblock(const struct superblock*);
extern size_t format_write_bool(const bool*, size_t);
extern size_t format_write_inode(const struct inode*, size_t);
extern size_t format_write_char(const char*, size_t);
extern size_t format_write_directory_item(const struct directory_item*, size_t);
extern void format_root_bm_off();

/*
 * Check if string is convertible to number.
 * I made this function, because strtol() converts even '12ab' to '12'
 * or doesn't notify, that conversion can't be done.
 * How to recognize if string was "0" and "!?".
 */
static bool isnumeric(const char* num) {
	if (strlen(num) == 0)
		return false;	// empty string
	if (strlen(num) == 1 && isnegnum(num))
		return false;	// first, and only, char is minus
	if (!(isnegnum(num) || isdigit(num[0])))
		return false;	// first char is not minus and not number
	for (size_t i = 1; i < strlen(num); ++i) {
		if (!isdigit(num[i]))
			return false;
	}
	return true;
}

static void get_datetime(char* datetime) {
	time_t t = time(NULL);
	struct tm* tm_info = localtime(&t);
	strftime(datetime, LOG_DATETIME_LENGTH_, "%Y-%m-%d %H:%M:%S", tm_info);
}

static int parse_filesystem_size(const char* num_str, uint32_t* size) {
	long num = 0;

	if (strlen(num_str) == 0) {
		set_myerrno(Err_fs_size_none);
		goto fail;
	}
	if (!isnumeric(num_str)) {
		set_myerrno(Err_fs_size_nan);
		goto fail;
	}

	// convert number
	reset_myerrno();
	num = strtol(num_str, NULL, 10);

	if (is_error()) {
		set_myerrno(Err_fs_size_sys_range);
		goto fail;
	}
	if (!isinrange(num)) {
		set_myerrno(Err_fs_size_sim_range);
		goto fail;
	}

	*size = (uint32_t) num;
	return RETURN_SUCCESS;

fail:
	fprintf(stderr, "! use size range from 1 to %d [MB].\n", FS_SIZE_MAX);
	return RETURN_FAILURE;
}

static int init_superblock(const int size, const uint32_t block_cnt) {
	char datetime[LOG_DATETIME_LENGTH_] = {0};
	get_datetime(datetime);

	printf("init: superblock.. ");

	// inode bitmap is after superblock
	uint32_t addr_bm_in = sizeof(struct superblock);
	// data bitmap is after inode bitmap
	uint32_t addr_bm_dat = addr_bm_in + block_cnt;
	// inodes are after data bitmap
	uint32_t addr_in = addr_bm_dat + block_cnt;
	// data are at the end of filesystem -- there is unused space between inodes and data
	uint32_t addr_dat = addr_in + block_cnt * sizeof(struct inode);

	// init superblock variables
	strncpy(sb.signature, "kmat95", sizeof(sb.signature) - 1);
	sprintf(sb.volume_descriptor, "%s, made by matenestor", datetime);
	sb.disk_size = size;
	sb.block_size = FS_BLOCK_SIZE;
	sb.block_count = block_cnt;
	sb.count_links = sb.block_size / sizeof(uint32_t);
	sb.count_dir_items = sb.block_size / sizeof(struct directory_item);
	sb.addr_bm_inodes = addr_bm_in;
	sb.addr_bm_data = addr_bm_dat;
	sb.addr_inodes = addr_in;
	sb.addr_data = addr_dat;

	// write superblock to file
	fs_write_superblock(&sb);
	fs_flush();
	puts("done");

	return RETURN_SUCCESS;
}

static int init_bitmap(const size_t block_cnt) {
	size_t i, batch;
	size_t loops = block_cnt / CACHE_SIZE;
	size_t over_fields = block_cnt % CACHE_SIZE;
	bool bitmap[CACHE_SIZE];

	printf("init: bitmap.. ");

	// all field are available
	memset(bitmap, true, CACHE_SIZE);

	// init rest of the bitmap
	for (i = 0; i <= loops; ++i) {
		batch = i < loops ? CACHE_SIZE : over_fields;
		format_write_bool(bitmap, batch);
	}

	fs_flush();
	puts("done");

	return RETURN_SUCCESS;
}

static int init_inodes(const size_t block_cnt) {
	size_t i, j;
	// how many inodes can be read into 'CACHE_SIZE'
	size_t cache_capacity = CACHE_SIZE / sizeof(struct inode);
	// block_cnt == total inodes count
	size_t loops = block_cnt / cache_capacity;
	size_t over_inodes = block_cnt % cache_capacity;
	// count of inodes to be read
	size_t batch = loops > 0 ? cache_capacity : over_inodes;
	// inode template
	struct inode inode_init;
	// array of cached inode in filesystem (inodes count == block count)
	struct inode inodes[batch];

	printf("init: inodes.. ");

	// init empty inode
	inode_init.id_inode = FREE_LINK;
	inode_init.inode_type = Inode_type_free;
	inode_init.file_size = 0;
	for (i = 0; i < COUNT_DIRECT_LINKS; ++i) {
		inode_init.direct[i] = FREE_LINK;
	}
	for (i = 0; i < COUNT_INDIRECT_LINKS_1; ++i) {
		inode_init.indirect_1[i] = FREE_LINK;
	}
	for (i = 0; i < COUNT_INDIRECT_LINKS_2; ++i) {
		inode_init.indirect_2[i] = FREE_LINK;
	}

	// cache inodes to local array for future FS_WRITE
	for (i = 0; i < batch; ++i) {
		memcpy(&inodes[i], &inode_init, sizeof(struct inode));
	}

	// write everything
	for (i = 0; i <= loops; ++i) {
		batch = i < loops ? cache_capacity : over_inodes;
		// initialize new inode ids
		// NOTE: inode ids start at 1!
		for (j = 0; j < batch; ++j) {
			// really 'cache_capacity', not 'batch',
			// because of last cycle, where 'batch' == 'over_inodes
			inodes[j].id_inode = (j+1) + i*cache_capacity;
		}
		format_write_inode(inodes, batch);
	}

	fs_flush();
	puts("done");

	return RETURN_SUCCESS;
}

static int init_blocks(const uint32_t fs_size) {
	size_t i, batch;
	// how much bytes is missing till end of filesystem
	// after meta part -- empty space part + data part
	uint64_t remaining_part = mb2b(fs_size) - ftell(filesystem);
	size_t loops = remaining_part / CACHE_SIZE;
	// helper array to be filled from
	char zeros[CACHE_SIZE] = {0};
	// for printing percentage, so it doesn't look like nothing is happening
	size_t percent5 = loops / 20 + 1;

	// fill rest of filesystem with batches of zeros
	for (i = 0; i <= loops; ++i) {
		batch = i < loops ? CACHE_SIZE : remaining_part % CACHE_SIZE;
		format_write_char(zeros, batch);

		if (i % percent5 == 0)
			printf("\rinit: data blocks.. %d %%", (int) (((float) i / loops)*100));
	}
	fs_flush();
	puts("\rinit: data blocks.. done");

	return RETURN_SUCCESS;
}

static int init_root() {
	struct inode inode_root = {0};
	// root inode has id=1 and is on filesystem index=1 (0 is nothing)
	struct directory_item dir_root[sb.count_dir_items];

	printf("init: root.. ");

	// root inode
	fs_read_inode(&inode_root, 1, 1);
	inode_root.inode_type = Inode_type_dirc;
	inode_root.file_size = FS_BLOCK_SIZE;
	inode_root.direct[0] = 1;
	// root directory
	init_empty_dir_block(dir_root, 1, 1);

	// turn off root bitmap fields
	format_root_bm_off();
	// write root inode
	fs_write_inode(&inode_root, 1, 1);
	// write root data block ('/' dir)
	fs_write_directory_item(dir_root, 2, 1);

	// cache root inode to simulation cache
	memcpy(&in_actual, &inode_root, sizeof(struct inode));
	puts("done");

	return RETURN_SUCCESS;
}

int sim_format(const char* fs_size_str, const char* path) {
	int ret = RETURN_FAILURE;
	// necessary step with double type variable for precision
	double dt_percentage = 0;
	uint32_t block_cnt = 0, fs_size = 0;

	log_info("Formatting filesystem [path: %s] [size: %s]", path, fs_size_str);

	if (parse_filesystem_size(fs_size_str, &fs_size) == RETURN_SUCCESS) {
		// Count of blocks in data blocks part is equal to bitmaps sizes and count of inodes.
		// 'fs_size' is in MB, 'block_size' is in B, data part is 100*'PERCENTAGE' % of whole filesystem
		dt_percentage = mb2b(fs_size) * PERCENTAGE;
		block_cnt = (uint32_t) (dt_percentage / FS_BLOCK_SIZE);

		if ((filesystem = fopen(path, "wb+")) != NULL) {
			init_superblock(fs_size, block_cnt);
			init_bitmap(block_cnt); // inodes
			init_bitmap(block_cnt); // data blocks
			init_inodes(block_cnt);
			init_blocks(fs_size);
			init_root();

			printf("format: filesystem formatted, size: %d MB\n", fs_size);
			log_info("format: Filesystem [%s] with size [%d MB] formatted.", path, fs_size);
			ret = RETURN_SUCCESS;
		}
	} else {
		log_error("Simulation error while formatting: %s", my_strerror(my_errno));
	}

	return ret;
}
