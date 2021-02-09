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
	static char units[][3] = {"B", "KB", "MB", "GB"};

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
	strncpy(block[0].item_name, ".", 1);
	strncpy(block[1].item_name, "..", 2);

	return RETURN_SUCCESS;
}

/*
 * Get count of data blocks in filesystem, which is needed,
 * in order to create new file inode.
 */
uint32_t get_count_data_blocks(const off_t file_size) {
	if (file_size % sb.block_size != 0)
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
						log_info("Got item id [%d] by name [%s].", carry->id, carry->name);
						ret = true;
					}
					break;
				case search_name:
					if (block[j].id_inode == carry->id) {
						strncpy(carry->name, block[j].item_name, STRLEN_ITEM_NAME);
						log_info("Got item name [%s] by id [%d].", carry->name, carry->id);
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
			if (block[j].id_inode == carry->id) {
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
				printf("- %4.1f%s\t%s\n", file_size, unit, block[j].item_name);
			}
			else if (inode_ls.inode_type == Inode_type_dirc) {
				printf("d %4.1f%s\t%s%s\n", file_size, unit, block[j].item_name, SEPARATOR);
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
	size_t i;
	char block[sb.block_size];
	struct carry_stream* carry = (struct carry_stream*) p_carry;

	for (i = 0; i < links_count; ++i) {
		if (links[i] == FREE_LINK)
			continue;

		if (!feof(carry->file)) {
			stream_incp(block, carry->file);
			fs_write_data(block, sb.block_size, links[i]);
		} else {
			return true;
		}
	}
	return false;
}

/*
 * In-copy data inplace, when there is access to data blocks directly via links.
 */
inline int incp_data_inplace(const uint32_t* links, const uint32_t links_count, FILE* file) {
	size_t i;
	char block[sb.block_size];

	for (i = 0; i < links_count; ++i) {
		if (!feof(file)) {
			stream_incp(block, file);
			fs_write_data(block, sb.block_size, links[i]);
		}
	}
	return RETURN_SUCCESS;
}

/*
 * Out-copy inode data to system file.
 */
ITERABLE(outcp_data) {
	size_t i;
	char block[sb.block_size];
	struct carry_stream* carry = (struct carry_stream*) p_carry;

	for (i = 0; i < links_count; ++i) {
		if (links[i] == FREE_LINK)
			continue;

		fs_read_data(block, sb.block_size, links[i]);
		stream_outcp(block, carry->file);
	}
	// always return false, so all links are iterated
	return false;
}

// PROBABLY NOT NEEDED AT ALL -------------------------------------------------

///*
// *  Get count of links in indirect links block.
// */
//  // TODO if needed, check whole block
//size_t get_count_links(int32_t* source) {
//	size_t items = 0;
//	int32_t* p_link = source;
//
//	while (*p_link != FREE_LINK && items < sb.count_links) {
//		++p_link;
//		++items;
//	}
//
//	return items;
//}
//
///*
// *  Get count of directory records in directory block.
// */
//  // TODO if needed, check whole block
//size_t get_count_dirs(struct directory_item* source) {
//	size_t items = 0;
//	struct directory_item* p_dir = source;
//
//	while (strcmp(p_dir->item_name, "") != 0 && items < sb.count_dir_items) {
//		++p_dir;
//		++items;
//	}
//
//	return items;
//}
//
///*
// *  Check if block is full of directory items.
// */
//static bool is_block_full_dirs(const int32_t id_block) {
//	struct directory_item block[sb.count_dir_items];
//	fs_read_directory_item(block, sb.count_dir_items, id_block);
//	return (bool) (get_count_dirs(block) == sb.count_dir_items);
//}
//
///*
// *  Check if block is full of links.
// */
//static bool is_block_full_links(const int32_t id_block) {
//	int32_t block[sb.count_links];
//	fs_read_link(block, sb.count_links, id_block);
//	return (bool) (get_count_links(block) == sb.count_links);
//}
