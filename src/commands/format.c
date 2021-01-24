#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "fs_cache.h"
#include "inode.h"
#include "../fs_operations.h"

#include "../../include/logger.h"
#include "../../include/errors.h"

#define LOG_DATETIME_LENGTH_	25
#define FS_SIZE_MAX				4095	// maximal filesystem size in megabytes MB (using 32b integers)
#define FS_BLOCK_SIZE			1024	// filesystem block size in bytes B
#define PERCENTAGE				0.95	// percentage of space for data in filesystem
#define mb2b(mb)				((mb)*1024UL*1024UL)
#define isnegnum(cha)			(cha[0] == '-')
#define isinrange(n)			((n) > 0 && (n) <= FS_SIZE_MAX)


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
	time_t t;
	struct tm* tm_info;
	t = time(NULL);
	tm_info = localtime(&t);
	strftime(datetime, LOG_DATETIME_LENGTH_, "%Y-%m-%d %H:%M:%S", tm_info);
}

static int is_valid_size(const char* num_str, uint32_t* size) {
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

	*size = (size_t) num;
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
	sb.count_links = sb.block_size / sizeof(int32_t);
	sb.count_dir_items = sb.block_size / sizeof(struct directory_item);
	sb.addr_bm_inodes = addr_bm_in;
	sb.addr_bm_data = addr_bm_dat;
	sb.addr_inodes = addr_in;
	sb.addr_data = addr_dat;

	// write superblock to file
	fs_write_superblock(&sb, sizeof(struct superblock), 1);
	fs_flush();
	puts("done");

	return RETURN_SUCCESS;
}

static int init_bitmap(const size_t block_cnt) {
	size_t i;
	size_t loops = block_cnt / CACHE_SIZE;
	size_t over_fields = block_cnt % CACHE_SIZE;
	// count of field to be read
	size_t batch = loops > 0 ? CACHE_SIZE : over_fields;
	bool bitmap[batch];

	printf("init: bitmap.. ");

	// first loop is done manually, because very first field is set to 'false'
	memset(bitmap, true, batch);
	bitmap[0] = false;
	fs_write_bool(bitmap, sizeof(bool), batch);
	bitmap[0] = true;

	// init rest of the bitmap
	for (i = 1; i <= loops; ++i) {
		batch = i < loops ? CACHE_SIZE : over_fields;
		fs_write_bool(bitmap, sizeof(bool), batch);
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
	struct inode in;
	// array of cached inode in filesystem (inodes count == block count)
	struct inode inodes[cache_capacity];

	printf("init: inodes.. ");

	// --- init root inode

	in.id_inode = 0;
	in.item_type = Itemtype_directory;
	in.file_size = FS_BLOCK_SIZE;
	// root inode point to data block on index 0
	in.direct[0] = 0;
	// set links root inode
	for (i = 1; i < COUNT_DIRECT_LINKS; ++i) {
		in.direct[i] = FREE_LINK;
	}
	for (i = 0; i < COUNT_INDIRECT_LINKS_1; ++i) {
		in.indirect1[i] = FREE_LINK;
	}
	for (i = 0; i < COUNT_INDIRECT_LINKS_2; ++i) {
		in.indirect2[i] = FREE_LINK;
	}

	// cache root inode to local array for future FS_WRITE
	memcpy(&inodes[0], &in, sizeof(struct inode));
	// and to simulator cache
	memcpy(&in_actual, &in, sizeof(struct inode));

	// --- set free inodes

	// reset values for rest of the inodes
	in.item_type = Itemtype_free;
	in.file_size = 0;
	in.direct[0] = FREE_LINK;

	// cache rest of inodes to local array for future FS_WRITE
	for (i = 1; i < batch; ++i) {
		in.id_inode = i;
		memcpy(&inodes[i], &in, sizeof(struct inode));
	}

	// --- write everything

	// first loop is done manually, because very first inode is root
	fs_write_inode(inodes, sizeof(struct inode), batch);

	// rewrite first 'root' inode in cache array to free inode
	memcpy(&inodes[0], &in, sizeof(struct inode));

	// write rest of the inodes
	for (i = 1; i <= loops; ++i) {
		batch = i < loops ? cache_capacity : over_inodes;

		// initialize new inode ids
		for (j = 0; j < batch; ++j) {
			// really 'cache_capacity', not 'batch'
			inodes[j].id_inode = j + i*cache_capacity;
		}

		fs_write_inode(inodes, sizeof(struct inode), batch);
	}

	fs_flush();
	puts("done");

	return RETURN_SUCCESS;
}


static int init_blocks(const uint32_t fs_size) {
	size_t i;
	// how much bytes is missing till end of filesystem
	// after meta part -- empty space part + data part
	uint32_t remaining_part = mb2b(fs_size) - ftell(filesystem);
	// helper array to be filled from
	char zeros[CACHE_SIZE] = {0};

	// variables for printing percentage, so it doesn't look like nothing is happening
	size_t loops = remaining_part / CACHE_SIZE;
	size_t percent5 = loops / 20 + 1;

	printf("init: data blocks.. 0 %%");

	// fill rest of filesystem with batches of zeros
	for (i = 1; i <= loops; ++i) {
		fs_write_char(zeros, sizeof(char), CACHE_SIZE);

		if (i % percent5 == 0)
			printf("\rinit: data block.. %d %%", (int) (((float) i / loops)*100));
	}
	// fill zeros left till very end of filesystem (over fields)
	fs_write_char(zeros, sizeof(char), remaining_part % CACHE_SIZE);
	fs_flush();
	puts("\rinit: data blocks.. done");

	return RETURN_SUCCESS;
}


static int init_root_dir() {
	// root directory items
	struct directory_item di[2] = {0};

	// init directories in root directory (fk_id_inode is already set to 0 by init)
	strncpy(di[0].item_name, ".", 1);
	strncpy(di[1].item_name, "..", 2);

	// write root directory items to file
	fs_seek_set(sb.addr_data);
	fs_write_directory_item(di, sizeof(struct directory_item), 2);
	fs_flush();

	return RETURN_SUCCESS;
}


int format_(const char* fs_size_str, const char* path) {
	int ret = RETURN_FAILURE;
	// necessary step with double type variable for precision
	double dt_percentage = 0;
	uint32_t block_cnt = 0, fs_size = 0;

	log_info("Formatting filesystem [path: %s] [size: %s]", path, fs_size_str);

	if (is_valid_size(fs_size_str, &fs_size) == RETURN_SUCCESS) {
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
			init_root_dir();

			printf("format: filesystem formatted, size: %d MB\n", fs_size);
			log_info("format: Filesystem [%s] with size [%d MB] formatted.", path, fs_size);
			ret = RETURN_SUCCESS;
		}
	} else {
		log_error("Simulation error while formatting: %s", my_strerror(my_errno));
	}

	return ret;
}
