#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "fs_prompt.h"
#include "inode.h"
#include "iteration_carry.h"

#include "errors.h"
#include "logger.h"


// system io
extern size_t stream_incp(char* buffer, const size_t count, FILE* stream);
extern size_t stream_outcp(const char* buffer, const size_t count, FILE* stream);

enum search_for {
	search_id,
	search_name,
};

/*
 * Transform number to human readable form and choose correct unit for it.
 */
static float human_readable(char* unit, const uint32_t file_size_) {
	static short loop;
	static float file_size;
	static char units[][3] = {"B ", "KB", "MB", "GB"};

	loop = 0;
	file_size = file_size_;

	while (file_size > 1023.9f) {
		file_size /= 1024.0f;
		++loop;
	}
	strncpy(unit, units[loop], 3);
	return file_size;
}

/*
 * Init directory block inside filesystem with empty directories.
 */
int init_block_with_directories(const uint32_t id_block) {
	size_t i;
	struct directory_item block[sb.count_dir_items];

	for (i = 0; i < sb.count_dir_items; i++) {
	    block->id_inode = FREE_LINK;
		strncpy(block->item_name, "", STRLEN_ITEM_NAME);
	}

	fs_write_directory_item(block, sb.count_dir_items, id_block);
	return RETURN_SUCCESS;
}

/*
 * Init given directory block with dot dirs
 * and rest of space with empty directories.
 */
int init_empty_dir_block(struct directory_item* block,
						 const uint32_t id_self, const uint32_t id_parent) {

	for (size_t i = 0; i < sb.count_dir_items; i++) {
		block[i].id_inode = FREE_LINK;
		strncpy(block[i].item_name, "", STRLEN_ITEM_NAME);
	}
	block[0].id_inode = id_self;
	block[1].id_inode = id_parent;
	strncpy(block[0].item_name, ".", 2);
	strncpy(block[1].item_name, "..", 3);

	return RETURN_SUCCESS;
}

/*
 *  Get count of links in deep block.
 */
size_t get_count_links(const uint32_t* links) {
	size_t i;
	size_t links_count = 0;

	for (i = 0; i < sb.count_links; ++i) {
		if (links[i] == FREE_LINK)
			continue;
		links_count++;
	}
	return links_count;
}

static bool search_block(const enum search_for search, const uint32_t* links,
						 const size_t links_count, void* p_carry) {
	bool ret = false;
	size_t i, j;
	struct carry_dir_item* carry = (struct carry_dir_item*) p_carry;
	struct directory_item block[sb.count_dir_items];

	// check every link to block with directory items
	for (i = 0; i < links_count && !ret; ++i) {
		// other links are free
		if (links[i] == FREE_LINK) {
			continue;
		}
		fs_read_directory_item(block, sb.count_dir_items, links[i]);

		// search directory items for wanted element
		for (j = 0; j < sb.count_dir_items && !ret; ++j) {
			switch (search) {
				case search_id:
					if (strcmp(carry->name, block[j].item_name) == 0) {
						carry->id = block[j].id_inode;
						ret = true;
					}
					break;
				case search_name:
					if (block[j].id_inode == carry->id) {
						strncpy(carry->name, block[j].item_name, STRLEN_ITEM_NAME);
						ret = true;
					}
					break;
				default:
					set_myerrno(Err_fs_error);
					ret = true;
			}
		}
	}
	return ret;
}

/*
 *  Search whole block with directory items for 'id' of inode with 'name'.
 */
ITERABLE(search_block_inode_id) {
	return search_block(search_id, links, links_count, p_carry);
}

/*
 *  Search whole block with directory items for 'name' of inode with 'id'.
 */
ITERABLE(search_block_inode_name) {
	return search_block(search_name, links, links_count, p_carry);
}

/*
 * Add directory item to block.
 */
ITERABLE(add_block_item) {
	size_t i, j;
	struct directory_item block[sb.count_dir_items];
	struct carry_dir_item* carry = (struct carry_dir_item*) p_carry;

	for (i = 0; i < links_count; ++i) {
		if (links[i] == FREE_LINK)
			continue;

		fs_read_directory_item(block, sb.count_dir_items, links[i]);

		for (j = 0; j < sb.count_dir_items; ++j) {
			// empty place for new item record found
			if (block[j].id_inode == FREE_LINK) {
				block[j].id_inode = carry->id;
				strncpy(block[j].item_name, carry->name, strlen(carry->name) + 1);
				fs_write_directory_item(block, sb.count_dir_items, links[i]);
				return true;
			}
		}
	}
	return false;
}

/*
 * Delete directory item from block.
 */
ITERABLE(delete_block_item) {
	size_t i, j;
	struct directory_item block[sb.count_dir_items];
	struct carry_dir_item* carry = (struct carry_dir_item*) p_carry;

	for (i = 0; i < links_count; ++i) {
		if (links[i] == FREE_LINK)
			continue;

		fs_read_directory_item(block, sb.count_dir_items, links[i]);

		for (j = 0; j < sb.count_dir_items; ++j) {
			// record with id to delete found
			if (block[j].id_inode == carry->id && strcmp(block[j].item_name, carry->name) == 0) {
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
 * Check if there are other directories than "." and ".." in given blocks.
 */
ITERABLE(has_common_directories) {
	size_t i, j;
	struct directory_item block[sb.count_dir_items];

	for (i = 0; i < links_count; ++i) {
		if (links[i] == FREE_LINK)
			continue;

		fs_read_directory_item(block, sb.count_dir_items, links[i]);

		for (j = 0; j < sb.count_dir_items; ++j) {
			// other directory than "." and ".." found == directory is not empty
			if (strcmp(block[j].item_name, "") != 0
					&& strcmp(block[j].item_name, ".") != 0
					&& strcmp(block[j].item_name, "..") != 0) {
				return true;
			}
		}
	}
	return false;
}

/*
 * Check if there is space in directory inode for subdirectory record.
 */
ITERABLE(has_space_for_dir) {
	size_t i, j;
	struct directory_item block[sb.count_dir_items];

	for (i = 0; i < links_count; ++i) {
		// empty link in inode == new link to empty block can be created
		if (links[i] == FREE_LINK)
			return true;

		fs_read_directory_item(block, sb.count_dir_items, links[i]);

		for (j = 0; j < sb.count_dir_items; ++j) {
			// empty record found == space for new one
			if (strcmp(block[j].item_name, "") == 0) {
				return true;
			}
		}
	}
	return false;
}

/*
 * List all items in given blocks.
 */
ITERABLE(list_items) {
	size_t i, j;
	char unit[3] = {0};
	float file_size = 0;
	struct inode inode_ls = {0};
	struct directory_item block[sb.count_dir_items];

	for (i = 0; i < links_count; ++i) {
		if (links[i] == FREE_LINK)
			continue;

		fs_read_directory_item(block, sb.count_dir_items, links[i]);

		for (j = 0; j < sb.count_dir_items; ++j) {
			if (block[j].id_inode == FREE_LINK)
				continue;

			// read inodes in directory, so it is possible to print
			// their size and if item is file or directory
			fs_read_inode(&inode_ls, 1, block[j].id_inode);
			file_size = human_readable(unit, inode_ls.file_size);

			if (inode_ls.inode_type == Inode_type_file) {
				printf("- %6.1f%s\t%s\n", file_size, unit, block[j].item_name);
			}
			else if (inode_ls.inode_type == Inode_type_dirc) {
				printf("d %6.1f%s\t%s%s\n", file_size, unit, block[j].item_name, SEPARATOR);
			}
		}
	}
	// always return false, so all links are iterated
	return false;
}

/*
 * In-copy inode data from system file.
 */
ITERABLE(incp_data) {
	bool ret = false;
	size_t i, read;
	char block[sb.block_size];
	struct carry_stream* carry = (struct carry_stream*) p_carry;

	for (i = 0; i < links_count && !ret; ++i) {
		if (links[i] == FREE_LINK)
			continue;

		if (!feof(carry->file)) {
			read = stream_incp(block, sb.block_size, carry->file);
			// in case last block of data has smaller amount, fill rest with zeros
			// and write, so there are no leftover data
			if (read < sb.block_size) {
				memset(block + read, '\0', sb.block_size - read);
				ret = true;
			}
			fs_write_data(block, sb.block_size, links[i]);
		}
		else {
			// this else is probably redundant, since feof() is met,
			// when fread() reads less than 'sb.block_size'
			ret = true;
		}
	}
	return ret;
}

/*
 * In-copy data inplace, when there is access to data blocks directly via links.
 */
int incp_data_inplace(const uint32_t* links, const uint32_t links_count, FILE* file) {
	size_t i;
	size_t read = sb.block_size;
	char block[sb.block_size];

	for (i = 0; i < links_count; ++i) {
		// because 'links_count' is calculated exactly according to
		// size of given file, this should be always true
		if (!feof(file)) {
			read = stream_incp(block, sb.block_size, file);
			fs_write_data(block, read, links[i]);
		}
	}

	// in case last block of data has smaller amount, fill rest with zeros
	// and write, so there are no leftover data
	// condition is not in loop, because it is sure, that it will be very last link,
	// which will need this check... not like in 'incp_data', where is checked
	// block of links, so 'i' is iterated further and index of last link could get lost
	if (read < sb.block_size) {
		memset(block + read, '\0', sb.block_size - read);
		fs_write_data(block, sb.block_size, links[i - 1]);
	}
	return RETURN_SUCCESS;
}

/*
 * Out-copy inode data to system file.
 */
ITERABLE(outcp_data) {
	bool ret = false;
	size_t i;
	size_t to_write = 0;
	char block[sb.block_size];
	struct carry_stream* carry = (struct carry_stream*) p_carry;

	for (i = 0; i < links_count && carry->data_count > 0; ++i) {
		if (links[i] == FREE_LINK)
			continue;

		// set amount of data necessary to write
		if (carry->data_count > sb.block_size) {
			to_write = sb.block_size;
			carry->data_count -= sb.block_size;
		}
		else {
			to_write = carry->data_count;
			carry->data_count = 0;
			ret = true;
		}

		fs_read_data(block, sb.block_size, links[i]);
		stream_outcp(block, to_write, carry->file);
	}
	// always return false, so all links are iterated
	return ret;
}

/*
 * Concatenate data block and print the result.
 */
ITERABLE(cat_data) {
	size_t i;
	char block[sb.block_size + 1];
	block[sb.block_size] = '\0';

	for (i = 0; i < links_count; ++i) {
		if (links[i] == FREE_LINK)
			continue;

		fs_read_data(block, sb.block_size, links[i]);
		printf("%s", block);
	}
	// always return false, so all links are iterated
	return false;
}

/*
 * Copy data from one inode to another.
 */
ITERABLE(copy_data) {
	bool ret = false;
	size_t i;
	char block[sb.block_size];
	struct carry_copy* carry = (struct carry_copy*) p_carry;

	for (i = 0; i < links_count && carry->links_count > 0; ++i) {
		if (links[i] == FREE_LINK)
			continue;

		fs_read_data(block, sb.block_size, links[i]);
		fs_write_data(block, sb.block_size, *(carry->dest_links));
		// move pointer to another link
		carry->dest_links++;
		// decrement remaining links of data to copy
		carry->links_count--;

		if (carry->links_count == 0)
			ret = true;
	}
	// always return false, so all links are iterated
	return ret;
}
