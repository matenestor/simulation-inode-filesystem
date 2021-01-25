#include <stdint.h>
#include <stdbool.h>
#include <memory.h>

#include "fs_bitmap.h"
#include "fs_cache.h"
#include "fs_io.h"

#include "errors.h"
#include "logger.h"


static const bool t = true;
static const bool f = false;

void bitmap_field_inodes_on(const int32_t index) {
	fs_seek_set(sb.addr_bm_inodes + index);
	fs_write_bool(&t, 1);
	fs_flush();
}

void bitmap_field_inodes_off(const int32_t index) {
	fs_seek_set(sb.addr_bm_inodes + index);
	fs_write_bool(&f, 1);
	fs_flush();
}

void bitmap_field_blocks_on(const int32_t index) {
	fs_seek_set(sb.addr_bm_data + index);
	fs_write_bool(&t, 1);
	fs_flush();
}

void bitmap_field_blocks_off(const int32_t index) {
	fs_seek_set(sb.addr_bm_data + index);
	fs_write_bool(&f, 1);
	fs_flush();
}

/*
 *  Finds first empty field in bitmap on given address. (Either inodes or data blocks bitmap).
 *  If no field is available, error is set.
 *  Returns index number of empty bitmap field, or 'RETURN_FAILURE'.
 */
int32_t get_empty_bitmap_field(const int32_t address) {
	size_t i, j;
	size_t index = RETURN_FAILURE;
	size_t loops = sb.block_count / CACHE_SIZE;
	size_t over_fields = sb.block_count % CACHE_SIZE;
	// count of field to be read
	size_t batch = loops > 0 ? CACHE_SIZE : over_fields;
	bool bitmap[batch];

	memset(bitmap, false, batch);

	for (i = 0; i <= loops; ++i) {
		batch = i < loops ? CACHE_SIZE : over_fields;

		// cache part of bitmap
		fs_seek_set(address + i * CACHE_SIZE);
		fs_read_bool(bitmap, batch);

		// check cached array for a free field
		for (j = 0; j < batch; ++j) {
			if (bitmap[j]) {
				index = i * CACHE_SIZE + j;
				if (address == sb.addr_bm_inodes) {
					bitmap_field_inodes_off(index);
				} else {
					bitmap_field_blocks_off(index);
				}
				break;
			}
		}

		// if free field was found, break
		if (index != RETURN_FAILURE) {
			log_info("Free block, type: [%s], index: [%d].", address == sb.addr_bm_inodes ? "inodes" : "data", index);

			break;
		}
	}

	// no more free inodes/data blocks
	if (index == RETURN_FAILURE) {
		if (address == sb.addr_bm_inodes) {
			set_myerrno(Err_inode_no_inodes);
			log_error("Out of inodes.");
		} else if (address == sb.addr_bm_data) {
			set_myerrno(Err_block_no_blocks);
			log_error("Out of data blocks.");
		} else {
			set_myerrno(Err_fs_error);
			log_error("Unknown address [%d]. Filesystem might be corrupted.", address);
		}
	}

	return index;
}
