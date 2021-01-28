#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "fs_api.h"
#include "fs_cache.h"

#include "errors.h"
#include "logger.h"


static const bool bm_true = true;
static const bool bm_false = false;

static int32_t get_empty_bitmap_field(void (*fs_seek)(), void(*bitmap_field_off)()) {
	size_t i;
	size_t index = RETURN_FAILURE;
	bool* bitmap = malloc(sb.block_count);

	if (bitmap) {
		fs_seek(0);
		fs_read_bool(bitmap, sb.block_count);

		// check cached array for a free field
		for (i = 0; i < sb.block_count; ++i) {
			if (bitmap[i]) {
				bitmap_field_off(index);
				index = i;
				break;
			}
		}
		free(bitmap);
	}

	return index;
}

int32_t get_bitmap_field_inode() {
	int32_t index = get_empty_bitmap_field(fs_seek_bm_inodes, bitmap_field_inodes_off);
	if (index == RETURN_FAILURE) {
		if (is_error()) {
			my_perror("System error:");
		}
		set_myerrno(Err_inode_no_inodes);
		log_error("Out of inodes.");
	} else {
		log_info("Free inode, index: [%d].", index);
	}
	return index;
}

int32_t get_bitmap_field_data() {
	int32_t index = get_empty_bitmap_field(fs_seek_bm_data, bitmap_field_data_off);
	if (index == RETURN_FAILURE) {
		if (is_error()) {
			my_perror("System error:");
		}
		set_myerrno(Err_block_no_blocks);
		log_error("Out of data blocks.");
	} else {
		log_info("Free data block, index: [%d].", index);
	}
	return index;
}

void bitmap_field_inodes_on(const int32_t index) {
	fs_seek_bm_inodes(index);
	fs_write_bool(&bm_true, 1);
	fs_flush();
}

void bitmap_field_inodes_off(const int32_t index) {
	fs_seek_bm_inodes(index);
	fs_write_bool(&bm_false, 1);
	fs_flush();
}

void bitmap_field_data_on(const int32_t index) {
	fs_seek_bm_data(index);
	fs_write_bool(&bm_true, 1);
	fs_flush();
}

void bitmap_field_data_off(const int32_t index) {
	fs_seek_bm_data(index);
	fs_write_bool(&bm_false, 1);
	fs_flush();
}
