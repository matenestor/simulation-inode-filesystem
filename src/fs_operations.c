#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "fs_operations.h"
#include "fs_cache.h"
#include "fs_prompt.h"
#include "inode.h"
#include "utils.h"

#include "../include/logger.h"
#include "../include/errors.h"


// prototypes of functions in this unit
static int search_block_inodeid(void*, int32_t*, const int32_t*, size_t);
static int search_block_inodename(void*, int32_t*, const int32_t*, size_t);
static int search_blockid_diritems(void*, int32_t*, const int32_t*, size_t);
static int search_blockid_links(void*, int32_t*, const int32_t*, size_t);
static int search_links(void*, int32_t*, const struct inode*, int (*)(void*, int32_t*, const int32_t*, const size_t));
static int clear_blocks(int32_t*, size_t);
static int clear_links(struct inode*);

static void bitmap_field_off(int32_t, int32_t);
static void bitmap_field_on(int32_t, int32_t);
static int32_t get_empty_bitmap_field(int32_t);

static int32_t init_link_();
static int32_t init_block_(int32_t);
static int32_t clear_block_(int32_t);
static int32_t create_direct();
static int32_t create_indirect_1(int32_t*);
static int32_t create_indirect_2(int32_t*);

static bool is_block_full_dirs(int32_t);
static bool is_block_full_links(int32_t);


// ================================================================================================
// START: Filesystem input/output functions.

void fs_seek_set(uint32_t offset) {
	if (offset > INT32_MAX) {
		offset -= INT32_MAX;
		fseek(filesystem, INT32_MAX, SEEK_SET);
		fseek(filesystem, offset, SEEK_CUR);
	}
	else {
		fseek(filesystem, offset, SEEK_SET);
	}
}

void fs_flush() {
	fflush(filesystem);
}

unsigned int fs_read_superblock(struct superblock* buffer, const size_t size, const size_t count) {
	return fread(buffer, size, count, filesystem);
}

unsigned int fs_read_inode(struct inode* buffer, const size_t size, const size_t count) {
	return fread(buffer, size, count, filesystem);
}

unsigned int fs_read_directory_item(struct directory_item* buffer, const size_t size, const size_t count) {
	return fread(buffer, size, count, filesystem);
}

unsigned int fs_read_int32t(int32_t* buffer, const size_t size, const size_t count) {
	return fread(buffer, size, count, filesystem);
}

unsigned int fs_read_bool(bool* buffer, const size_t size, const size_t count) {
	return fread(buffer, size, count, filesystem);
}

unsigned int fs_read_char(char* buffer, const size_t size, const size_t count) {
	return fread(buffer, size, count, filesystem);
}

unsigned int fs_write_superblock(const struct superblock* buffer, const size_t size, const size_t count) {
	return fwrite(buffer, size, count, filesystem);
}

unsigned int fs_write_inode(const struct inode* buffer, const size_t size, const size_t count) {
	return fwrite(buffer, size, count, filesystem);
}

unsigned int fs_write_directory_item(const struct directory_item* buffer, const size_t size, const size_t count) {
	return fwrite(buffer, size, count, filesystem);
}

unsigned int fs_write_int32t(const int32_t* buffer, const size_t size, const size_t count) {
	return fwrite(buffer, size, count, filesystem);
}

unsigned int fs_write_bool(const bool* buffer, const size_t size, const size_t count) {
	return fwrite(buffer, size, count, filesystem);
}

unsigned int fs_write_char(const char* buffer, const size_t size, const size_t count) {
	return fwrite(buffer, size, count, filesystem);
}

// END: Filesystem input/output functions.
// ================================================================================================


// ================================================================================================
// START: Filesystem init and close functions.

void init_filesystem(const char* fsp, bool* is_formatted) {
	log_info("Loading filesystem [%s].", fsp);

	// if filesystem exists, load it
	if (access(fsp, F_OK) == 0) {
		// filesystem is ready to be loaded
		if ((filesystem = fopen(fsp, "rb+")) != NULL) {
			// cache super block
			fs_read_superblock(&sb, sizeof(struct superblock), 1);
			// move to inodes location
			fs_seek_set(sb.addr_inodes);
			// cache root inode
			fs_read_inode(&in_actual, sizeof(struct inode), 1);

			*is_formatted = true;
			puts("Filesystem loaded successfully.");

			log_info("Filesystem [%s] loaded.", fsp);
		}
		// filesystem is ready to be loaded, but there was an system error
		else {
			*is_formatted = false;
			puts("Filesystem corrupted or error while loading it. "
				"Restart simulation or format it again.");
			my_perror("System error:");
			reset_myerrno();
		}
	}
	// else notify about possible formatting
	else {
		*is_formatted = false;
		puts("No filesystem with this name found. "
	   		"You can format one with command 'format <size>'.");

		log_info("Filesystem [%s] not found.", fsp);
	}
}

void close_filesystem() {
	if (filesystem != NULL) {
		fclose(filesystem);
		log_info("Filesystem closed.");
	}
}

// END: Filesystem init and close functions.
// ================================================================================================


// ================================================================================================
// START: Filesystem inode search functions.

/*
 *  Searches whole block with directory items for 'id' of inode with 'name'.
 *  When variable, which is looked for, is found, function returns 'RETURN_SUCCESS'.
 *
 *  'name'  -- Name of inode, which 'id' is searched for.
 *  'id'    -- Pointer to variable, where result will be stored.
 *  'links' -- Block with direct links pointing to blocks with directory items, which will be checked.
 *  'links_count' -- Count of links in 'links' block,
 */
static int search_block_inodeid(void* _name, int32_t* id, const int32_t* links, size_t links_count) {
	int ret = RETURN_FAILURE;
	size_t i, j, items;
	char* name = (char*) _name;

	// array with directory items in block
	struct directory_item block[sb.count_dir_items];

	// check every link to block with directory items
	for (i = 0; i < links_count; ++i) {
		// other links are free
		if (links[i] == FREE_LINK) {
			continue;
		}

		fs_seek_set(sb.addr_data + links[i] * sb.block_size);
		fs_read_directory_item(block, sizeof(struct directory_item), sb.count_dir_items);
		items = get_count_dirs(block);

		// loop over existing directory items
		for (j = 0; j < items; ++j) {
			// inode of directory item with name 'name' found
			if (strcmp(name, block[j].item_name) == 0) {
				*id = block[j].fk_id_inode;
				ret = RETURN_SUCCESS;

				log_info("Got item id [%d] by name [%s].", *id, name);

				break;
			}
		}
	}

	return ret;
}

/*
 *  Searches whole block with directory items for 'name' of inode with 'id'.
 *  When variable, which is looked for, is found, function returns 'RETURN_SUCCESS'.
 *
 *  'name'  -- Name of inode; a variable, where result will be stored.
 *  'id'    -- Id of inode, which is searched for.
 *  'links' -- Block with direct links pointing to blocks with directory items, which will be checked.
 *  'links_count' -- Count of links in 'links' block,
 */
static int search_block_inodename(void* _name, int32_t* id, const int32_t* links, const size_t links_count) {
	int ret = RETURN_FAILURE;
	size_t i, j, items;
	char* name = (char*) _name;

	// array with directory items in block
	struct directory_item block[sb.count_dir_items];

	// check every link to block with directory items
	for (i = 0; i < links_count; ++i) {
		// other links are free
		if (links[i] == FREE_LINK) {
			continue;
		}

		fs_seek_set(sb.addr_data + links[i] * sb.block_size);
		fs_read_directory_item(block, sizeof(struct directory_item), sb.count_dir_items);
		items = get_count_dirs(block);

		// loop over existing directory items
		for (j = 0; j < items; ++j) {
			// inode name with 'id' found
			if (block[j].fk_id_inode == *id) {
				strncpy(name, block[j].item_name, STRLEN_ITEM_NAME - 1);
				ret = RETURN_SUCCESS;

				log_info("Got item name [%s] by id [%d].", name, *id);

				break;
			}
		}
	}

	return ret;
}

static int search_blockid_diritems(void* _id_block, int32_t* id, const int32_t* links, size_t links_count) {
	int ret = RETURN_FAILURE;
	size_t i, j, items;
	int32_t* id_block = (int32_t*) _id_block;

	// array with directory items in block
	struct directory_item block[sb.count_dir_items];

	// check every link to block with directory items
	for (i = 0; i < links_count; ++i) {
		// other links are free
		if (links[i] == FREE_LINK) {
			continue;
		}
		// id of block is link in inode, not in its block
		else if (links[i] == *id) {
			*id_block = links[i];
			ret = RETURN_SUCCESS;
		}
		// id is in inode's blocks
		else {
			fs_seek_set(sb.addr_data + links[i] * sb.block_size);
			fs_read_directory_item(block, sizeof(struct directory_item), sb.count_dir_items);
			items = get_count_dirs(block);

			// loop over existing directory items
			for (j = 0; j < items; ++j) {
				// inode name with 'id' found
				if (block[j].fk_id_inode == *id) {
					*id_block = links[i];
					ret = RETURN_SUCCESS;

					break;
				}
			}
		}
	}

	return ret;
}

static int search_blockid_links(void* _id_block, int32_t* id, const int32_t* links, size_t links_count) {
	int ret = RETURN_FAILURE;
	size_t i, j, items;
	int32_t* id_block = (int32_t*) _id_block;

	// array with links in block
	int32_t block[sb.count_dir_items];

	// check every link to block with directory items
	for (i = 0; i < links_count; ++i) {
		// other links are free
		if (links[i] == FREE_LINK) {
			continue;
		}
		// id of block is link in inode, not in its block
		else if (links[i] == *id) {
			*id_block = links[i];
			ret = RETURN_SUCCESS;
		}
		// id is in inode's blocks
		else {
			fs_seek_set(sb.addr_data + links[i] * sb.block_size);
			fs_read_int32t(block, sizeof(int32_t), sb.count_links);
			items = get_count_links(block);

			// loop over existing directory items
			for (j = 0; j < items; ++j) {
				// inode name with 'id' found
				if (block[j] == *id) {
					*id_block = links[i];
					ret = RETURN_SUCCESS;

					break;
				}
			}
		}
	}

	return ret;
}

/*
 *  Function checks every available link in given inode 'source'.
 *  Start with direct links, and continue with indirect links level 1 and level 2, if nothing was found.
 *  It reads every block with direct links and then call s function 'search_block(...)',
 *  where the block and variables are finally processed.
 *
 *  'name'   -- name of inode, which is either searched for, or searched with
 *  'id'     -- id of inode, which is either searched with, or searched for
 *  'source' -- Inode, which is being checked.
 *  'search_block' -- pointer to function, which will be called for block searching
 */
static int search_links(void* name, int32_t* id, const struct inode* source,
						int (*search_block)(void*, int32_t*, const int32_t*, const size_t)) {
	int32_t ret = RETURN_FAILURE;
	size_t i, links_direct, links_indirect;

	// block with direct links to be checked
	int32_t block_direct[sb.count_links];
	// block with indirect links to be checked
	int32_t block_indirect[sb.count_links];

	// first -- check direct links
	ret = search_block(name, id, source->direct, COUNT_DIRECT_LINKS);

	// second -- check indirect links of 1st level
	// note: if count of indirect links lvl 1 needs to be bigger than 1 in future, this `if` is needed to be in loop
	if (source->indirect1[0] != FREE_LINK && ret == RETURN_FAILURE) {
		// read whole block with 1st level indirect links to blocks with data
		fs_seek_set(sb.addr_data + source->indirect1[0] * sb.block_size);
		fs_read_int32t(block_direct, sizeof(int32_t), sb.count_links);
		links_direct = get_count_links(block_direct);

		ret = search_block(name, id, block_direct, links_direct);
	}

	// third -- check indirect links of 2nd level
	// note: if count of indirect links lvl 2 needs to be bigger than 1 in future, this `if` is needed to be in loop
	if (source->indirect2[0] != FREE_LINK && ret == RETURN_FAILURE) {
		// read whole block with 2nd level indirect links to blocks with 1st level indirect links
		fs_seek_set(sb.addr_data + source->indirect2[0] * sb.block_size);
		fs_read_int32t(block_indirect, sizeof(int32_t), sb.count_links);
		links_indirect = get_count_links(block_indirect);

		// loop over each 2nd level indirect link to get blocks with 1st level indirect links
		for (i = 0; i < links_indirect; ++i) {
			fs_seek_set(sb.addr_data + block_indirect[i] * sb.block_size);
			fs_read_int32t(block_direct, sizeof(int32_t), sb.count_links);
			links_direct = get_count_links(block_direct);

			// name or id found
			if ((ret = search_block(name, id, block_direct, links_direct)) != RETURN_FAILURE) {
				break;
			}
		}
	}

	if (ret == RETURN_FAILURE) {
		set_myerrno(Err_fs_error);
	}

	return ret;
}

/*
 *  Reads inode to 'in_dest' by given path. If path starts with "/",
 *  then function searches from root inode, else from actual inode, where user is.
 *  Inodes to final one are read and traversed folder by folder in path.
 *
 *  If inode was found, return 'RETURN_SUCCESS" in 'ret'.
 */
int32_t get_inode_by_path(struct inode* in_dest, const char* path) {
	int32_t ret = RETURN_SUCCESS;
	int32_t id = 0;
	char* dir = NULL;
	char path_copy[strlen(path) + 1];

	strncpy(path_copy, path, strlen(path) + 1);

	// track path from root
	if (path_copy[0] == SEPARATOR[0]) {
		fs_seek_set(sb.addr_inodes);
		fs_read_inode(in_dest, sizeof(struct inode), 1);
	}
	// track path from actual directory
	else {
		fs_seek_set(sb.addr_inodes + in_actual.id_inode * sizeof(struct inode));
		fs_read_inode(in_dest, sizeof(struct inode), 1);
	}

	id = in_dest->id_inode;

	// parse path -- get first directory in given path
	dir = strtok(path_copy, SEPARATOR);

	// parse path -- get rest of the directories, if there are any
	while (dir != NULL) {
		// get inode of first directory in given path
		ret = search_links(dir, &id, in_dest, search_block_inodeid);

		// no item with parsed name found
		if (ret == RETURN_FAILURE) {
			set_myerrno(Err_item_not_exists);
			break;
		}
		// still some elements to parse -- continue
		else {
			fs_seek_set(sb.addr_inodes + id * sizeof(struct inode));
			fs_read_inode(in_dest, sizeof(struct inode), 1);
		}

		dir = strtok(NULL, SEPARATOR);
	}

	return ret;
}

/*
 *  Get path from root to actual inode by going back from it to root over parents.
 *  Cache child inode and get if of its parent. Then cache parent inode
 *  and get name of its child. After retrieving name of child, append it to final
 *  path with preceding separator.
 *  If path would be too long for pwd, write at the beginning "..", instead of rest of path.
 */
int32_t get_path_to_root(char* dest_path, const uint16_t length_new_path, bool* is_overflowed) {
	int32_t ret = RETURN_SUCCESS;
	int32_t size_used = length_new_path;
	uint16_t child_name_length = 0;
	int32_t id_child, id_parent = 0;
	struct inode in_tmp = {0};

	// buffer for names of directories on path to root
	char child_name[STRLEN_ITEM_NAME] = {0};
	// buffer for whole path to root
	char buffer[length_new_path];
	// move pointer to end of buffer
	char* p_buffer = buffer + length_new_path - 2;

	memset(buffer, '\0', length_new_path);

	// cache first inode id in path
	id_child = in_actual.id_inode;

	// while not in root inode
	while (id_child != 0) {
		// cache child inode
		fs_seek_set(sb.addr_inodes + id_child * sizeof(struct inode));
		fs_read_inode(&in_tmp, sizeof(struct inode), 1);

		// get id of parent inode
		search_links("..", &id_parent, &in_tmp, search_block_inodeid);

		// cache parent inode
		fs_seek_set(sb.addr_inodes + id_parent * sizeof(struct inode));
		fs_read_inode(&in_tmp, sizeof(struct inode), 1);

		// get name of child inode
		if (search_links(child_name, &id_child, &in_tmp, search_block_inodename) == RETURN_FAILURE) {
			ret = RETURN_FAILURE;
			break;
		}

		child_name_length = strlen(child_name);

		// write child name and separator to path
		if (size_used - (child_name_length + 1) > 1) {
			size_used -= (child_name_length + 1);

			// move before child name
			p_buffer -= child_name_length;
			// write name of child inode
			strncpy(p_buffer, child_name, child_name_length);
			--p_buffer;
			// write separator to path
			*p_buffer = SEPARATOR[0];

			// parent becomes a child
			id_child = in_tmp.id_inode;
		}
		// path to root is too long
		else {
			*is_overflowed = true;
			size_used = 0;
			--p_buffer;
			strncpy(p_buffer, "..", 2);
			break;
		}
	}

	if (ret == RETURN_SUCCESS) {
		// in case actual inode is root already, write root to path,
		// because it is possible that the while loop did not run
		if (size_used != 0) {
			*p_buffer = SEPARATOR[0];
		}

		// copy path to root to given buffer
		strncpy(dest_path, p_buffer, length_new_path);
	}

	return ret;
}

// END: Filesystem inode search functions.
// ================================================================================================

// ================================================================================================
// START: Filesystem bitmap managing functions.

static void bitmap_field_off(const int32_t address, const int32_t index) {
	bool f = false;
	fs_seek_set(address + index);
	fs_write_bool(&f, sizeof(bool), 1);

	fs_flush();
}


static void bitmap_field_on(const int32_t address, const int32_t index) {
	bool t = true;
	fs_seek_set(address + index);
	fs_write_bool(&t, sizeof(bool), 1);

	fs_flush();
}

/*
 *  Finds first empty field in bitmap on given address. (Either inodes or data blocks bitmap).
 *  If no field is available, error is set.
 * 
 *  Returns index number of empty bitmap field, or 'RETURN_FAILURE'.
 */
static int32_t get_empty_bitmap_field(const int32_t address) {
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
		fs_read_bool(bitmap, sizeof(bool), batch);

		// check cached array for a free field
		for (j = 0; j < batch; ++j) {
			// check if field is free
			if (bitmap[j]) {
				// position of field in whole bitmap
				index = i * CACHE_SIZE + j;
				// turn off the index
				bitmap_field_off(address, index);

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
		}
		else if (address == sb.addr_bm_data) {
			set_myerrno(Err_block_no_blocks);

			log_error("Out of data blocks.");
		}
		else {
			set_myerrno(Err_fs_error);

			log_error("Unknown address [%d]. Filesystem might be corrupted.", address);
		}
	}

	return index;
}

// END: Filesystem bitmap managing functions.
// ================================================================================================


// ================================================================================================
// START: Filesystem inode creation and destruction functions.

/*
 *  Create new inode for either directory or file. If inode is for directory,
 *  function also initializes '.' and '..' directories.
 *
 *  Returns index number of new inode, or 'RETURN_FAILURE'.
 */
int32_t create_inode(struct inode* new_inode, const enum item type, const int32_t id_parent) {
	int32_t ret = RETURN_FAILURE;
	// id of new inode, which will be used
	int32_t id_free_inode = RETURN_FAILURE;
	// id of new block, which will be used for the inode
	int32_t id_free_block = RETURN_FAILURE;
	// directory item record, in case new inode is for directory
	struct directory_item new_dir_item[2] = {0};

	// find free field for inode, which is also id of the inode
	id_free_inode = get_empty_bitmap_field(sb.addr_bm_inodes);
	// find free field for link of the inode
	id_free_block = get_empty_bitmap_field(sb.addr_bm_data);

	// if there is free inode and data block for it, use it
	if (!(id_free_inode == RETURN_FAILURE || id_free_block == RETURN_FAILURE)) {
		// cache the free inode, in order to init it
		fs_seek_set(sb.addr_inodes + id_free_inode * sizeof(struct inode));
		fs_read_inode(new_inode, sizeof(struct inode), 1);

		// init new inode
		new_inode->item_type = type;
		new_inode->direct[0] = id_free_block;

		if (type == Itemtype_directory) {
			new_inode->file_size = sb.block_size;

			// create . directory
			strcpy(new_dir_item[0].item_name, ".");
			new_dir_item[0].fk_id_inode = id_free_inode;

			// create .. directory
			strcpy(new_dir_item[1].item_name, "..");
			new_dir_item[1].fk_id_inode = id_parent;

			fs_seek_set(sb.addr_data + id_free_block * sb.block_size);
			fs_write_directory_item(new_dir_item, sizeof(struct directory_item), 2);

			fs_flush();
		}

		// write new updated inode
		fs_seek_set(sb.addr_inodes + id_free_inode * sizeof(struct inode));
		fs_write_inode(new_inode, sizeof(struct inode), 1);

		fs_flush();

		ret = id_free_inode;

		log_info("New inode created, type: [%s], id: [%d].", type == Itemtype_directory ? "directory" : "file", id_free_inode);
	}
	else {
		log_error("Unable to create new inode.");
	}

	return ret;
}

static int clear_blocks(int32_t* links, const size_t size) {
	size_t i;

	for (i = 0; i < size; ++i) {
		if (links[i] != FREE_LINK) {
			clear_block_(links[i]);
			links[i] = FREE_LINK;
		}
	}

	return 0;
}

static int clear_links(struct inode* source) {
	size_t i, links_direct, links_indirect;

	// block with direct links to be cleared
	int32_t block_direct[sb.count_links];
	// block with indirect links to be cleared
	int32_t block_indirect[sb.count_links];

	// first -- clear direct links
	clear_blocks(source->direct, COUNT_DIRECT_LINKS);

	// second -- clear indirect links of 1st level
	// note: if count of indirect links lvl 1 needs to be bigger than 1 in future, this `if` is needed to be in loop
	if (source->indirect1[0] != FREE_LINK) {
		// read whole block with 1st level indirect links to blocks with data
		fs_seek_set(sb.addr_data + source->indirect1[0] * sb.block_size);
		fs_read_int32t(block_direct, sizeof(int32_t), sb.count_links);
		links_direct = get_count_links(block_direct);

		// clear all data blocks, which every direct link in block points at
		clear_blocks(block_direct, links_direct);

		// clear indirect link lvl 1 in inode
		clear_block_(source->indirect1[0]);
		source->indirect1[0] = FREE_LINK;
	}

	// third -- clear indirect links of 2nd level
	// note: if count of indirect links lvl 2 needs to be bigger than 1 in future, this `if` is needed to be in loop
	if (source->indirect2[0] != FREE_LINK) {
		// read whole block with 2nd level indirect links to blocks with 1st level indirect links
		fs_seek_set(sb.addr_data + source->indirect2[0] * sb.block_size);
		fs_read_int32t(block_indirect, sizeof(int32_t), sb.count_links);
		links_indirect = get_count_links(block_indirect);

		// loop over each 2nd level indirect link to get blocks with 1st level indirect links
		for (i = 0; i < links_indirect; ++i) {
			fs_seek_set(sb.addr_data + block_indirect[i] * sb.block_size);
			fs_read_int32t(block_direct, sizeof(int32_t), sb.count_links);
			links_direct = get_count_links(block_direct);

			// clear all data blocks, which every direct link in block points at
			clear_blocks(block_direct, links_direct);

			// clear indirect link lvl 1 in inode
			clear_block_(block_indirect[i]);
			block_indirect[i] = FREE_LINK;
		}

		// clear indirect link lvl 2 in inode
		clear_block_(source->indirect2[0]);
		source->indirect2[0] = FREE_LINK;
	}

	return 0;
}

int delete_empty_links(int32_t* link, struct inode* source) {
	size_t i, items = 0;
	int32_t id_block, id_block2;
	int32_t block[sb.count_dir_items];

	// search for id of block, where link to empty block is located
	search_links(&id_block, link, source, search_blockid_links);

	fs_seek_set(sb.addr_data + id_block * sb.block_size);
	fs_read_int32t(block, sizeof(int32_t), sb.count_links);
	items = get_count_links(block);

	if (items == 1) {
		// todo clear_parent
		clear_block_(id_block);

		delete_empty_links(&id_block, source);
	}
	else {
		for (i = 0; i < items; ++i) {
			if (block[i] == *link) {
				for (; i < items - 1; ++i) {
					block[i] = block[i+1];
				}
				block[i] = FREE_LINK;

				break;
			}
		}
	}
}

int delete_record_from_parent(const uint32_t* id_child, struct inode* in_parent) {
	size_t i, items = 0;
	int32_t id_block = 0;
	struct directory_item block[sb.count_dir_items];
	struct directory_item empty_dir_item = {"", 0};

	// get 'id_block' in 'in_parent', where record of 'id_child' is
	search_links(&id_block, (int32_t*) id_child, in_parent, search_blockid_diritems);

	fs_seek_set(sb.addr_data + id_block * sb.block_size);
	fs_read_directory_item(block, sizeof(struct directory_item), sb.count_dir_items);
	items = get_count_dirs(block);

	if (items == 1) {
		// clear block with single directory record
		clear_block_(id_block);
		// delete the link from the block with links
		delete_empty_links(&id_block, in_parent);
	}
	else {
		for (i = 0; i < items; ++i) {
			if (block[i].fk_id_inode == *id_child) {
				for (; i < items - 1; ++i) {
					memcpy(&block[i], &block[i+1], sizeof(struct directory_item));
				}
				memcpy(&block[i], &empty_dir_item, sizeof(struct directory_item));

				break;
			}
		}
	}
}

/*
 * 	Destroys given inode by deleting record from its parent's blocks (if it is a directory),
 * 	resetting its values, clearing blocks and links, and turning on its bitmap field.
 */
int destroy_inode(struct inode* old_inode) {
	int32_t id_parent;
	struct inode in_parent = {0};
	struct directory_item block[sb.count_dir_items];

	// delete record about the directory from its parent
	if (old_inode->item_type == Itemtype_directory) {
		// read first block with records (should be the only link, when inode to delete is a directory)
		fs_seek_set(sb.addr_data + old_inode->direct[0] * sb.block_size);
		fs_read_directory_item(block, sizeof(struct directory_item), sb.count_dir_items);

		// ".." directory is on second place in records
		id_parent = block[1].fk_id_inode;

		// read parent inode
		fs_seek_set(sb.addr_inodes + id_parent * sizeof(struct inode));
		fs_read_inode(&in_parent, sizeof(struct inode), 1);

		// delete record of 'old_inode' in its parent
		delete_record_from_parent(&old_inode->id_inode, &in_parent);
		// =if record was last the block, delete the block
		delete_empty_links(0, &in_parent);
	}

	// reset attributes of inode
	old_inode->item_type = Itemtype_free;
	old_inode->file_size = 0;

	// clear all blocks that inode is pointing at
	clear_links(old_inode);

	// write cleared inode
	fs_seek_set(sb.addr_inodes + old_inode->id_inode * sizeof(struct inode));
	fs_write_inode(old_inode, sizeof(struct inode), 1);

	// turn on its bitmap field
	bitmap_field_on(sb.addr_bm_inodes, old_inode->id_inode);

	return 0;
}

// END: Filesystem inode creation and destruction functions.
// ================================================================================================


// ================================================================================================
// START: Filesystem creation of links in inode.

/*
 *  Initialize new link to empty data block.
 *  Return index number of the block, or 'RETURN_FAILURE'.
 */
static int32_t init_link_() {
	return get_empty_bitmap_field(sb.addr_bm_data);
}

/*
 *  Initialize block with 'FREE_LINK's and first link to empty block.
 *  Return index number of the block, or 'RETURN_FAILURE'.
 */
static int32_t init_block_(int32_t id_block) {
	int32_t free_index = RETURN_FAILURE;
	int32_t block[sb.count_links];

	if ((free_index = init_link_()) != RETURN_FAILURE) {
		// init the block
		memset(block, FREE_LINK, sb.count_links);
		block[0] = free_index;

		// write initialized block
		fs_seek_set(sb.addr_data + id_block * sb.block_size);
		fs_write_int32t(block, sizeof(int32_t), sb.count_links);

		fs_flush();
	}

	return free_index;
}

static int32_t clear_block_(int32_t id_block) {
	char block[sb.block_size];

	// clear the block
	memset(block, '\0', sb.block_size);

	// write initialized block
	fs_seek_set(sb.addr_data + id_block * sb.block_size);
	fs_write_char(block, sizeof(char), sb.block_size);

	fs_flush();

	bitmap_field_on(sb.addr_bm_data, id_block);

	return 0;
}

/*
 *  Create new direct link.
 *  Returns index number of block where new link points to, or 'RETURN_FAILURE'.
 */
static int32_t create_direct() {
	return init_link_();
}

/*
 *  Create new indirect link level 1.
 *  Returns index number of block where new link points to, or 'RETURN_FAILURE'.
 */
static int32_t create_indirect_1(int32_t* link) {
	int ret = RETURN_FAILURE;
	int32_t id_block_direct = RETURN_FAILURE;
	int32_t id_block_data = RETURN_FAILURE;

	// init link to block of direct links
	if ((id_block_direct = init_link_()) != RETURN_FAILURE) {
		// init block with direct links (return value is first link
		// with address to data block, or fail)
		if ((id_block_data = init_block_(id_block_direct)) != RETURN_FAILURE) {
			*link = id_block_direct;
			ret = id_block_data;
		}
		// block was not initialized == no more free blocks,
		// so turn on the block with direct links again
		else {
			bitmap_field_on(sb.addr_bm_data, id_block_direct);
		}
	}

	return ret;
}

/*
 *  Create new indirect link level 2.
 *  Returns index number of block where new link points to, or 'RETURN_FAILURE'.
 */
static int32_t create_indirect_2(int32_t* link) {
	int ret = RETURN_FAILURE;
	int32_t id_block_indirect = RETURN_FAILURE;
	int32_t id_block_direct = RETURN_FAILURE;
	int32_t id_block_data = RETURN_FAILURE;

	// init block of indirect links level 1
	if ((id_block_direct = create_indirect_1(&id_block_indirect)) != RETURN_FAILURE) {
		// init block with direct links (return value is first link
		// with address to data block, or fail)
		if ((id_block_data = init_block_(id_block_direct)) != RETURN_FAILURE) {
			*link = id_block_indirect;
			ret = id_block_data;
		}
		// blocks were not initialized == no more free blocks, so clear block with indirect links
		// level 1 and turn on both blocks with indirect links level 1 and direct links again
		else {
			clear_block_(id_block_indirect);
			bitmap_field_on(sb.addr_bm_data, id_block_indirect);
			bitmap_field_on(sb.addr_bm_data, id_block_direct);
		}
	}

	return ret;
}

// ================================================================================================

/*
 *  Check if block is full of directory items.
 *  Returns true if block is full, else false.
 */
static bool is_block_full_dirs(const int32_t id_block) {
	bool is_full = false;
	size_t items = 0;
	struct directory_item block[sb.count_dir_items];

	fs_seek_set(sb.addr_data + id_block * sb.block_size);
	fs_read_directory_item(block, sizeof(struct directory_item), sb.count_dir_items);
	items = get_count_dirs(block);

	// block is full of directory records
	if (items == sb.count_dir_items) {
		is_full = true;
	}

	return is_full;
}

/*
 *  Check if block is full of links.
 *  Returns true if block is full, else false.
 */
static bool is_block_full_links(const int32_t id_block) {
	bool is_full = false;
	size_t items = 0;
	int32_t block[sb.count_links];

	fs_seek_set(sb.addr_data + id_block * sb.block_size);
	fs_read_int32t(block, sizeof(int32_t), sb.count_links);
	items = get_count_links(block);

	// block is full of directory records
	if (items == sb.count_links) {
		is_full = true;
	}

	return is_full;
}

/*
 *  Get first available link in given inode, or create new link in the inode.
 *  Returns id of block where the link points to, or 'RETURN_FAILURE'.
 */
int32_t get_link(struct inode* source) {
	int32_t id_free_block = RETURN_FAILURE;
	size_t i;
	// flag for changes in 'inode', about changes in blocks takes care functions themselves
	bool is_new_link = false;

	// check direct links for free one
	for (i = 0; i < COUNT_DIRECT_LINKS; ++i) {
		// if link in inode is free, create new link
		if (source->direct[i] == FREE_LINK) {
			if ((id_free_block = create_direct()) != RETURN_FAILURE) {
				source->direct[i] = id_free_block;
				is_new_link = true;
			}
			break;
		}
		else {
			if (!is_block_full_dirs(source->direct[i])) {
				id_free_block = source->direct[i];
				break;
			}
		}
	}

	// if new link was created, write the change to filesystem
	if (is_new_link) {
		fs_seek_set(sb.addr_inodes + source->id_inode * sizeof(struct inode));
		fs_write_inode(source, sizeof(struct inode), 1);

		fs_flush();
	}

	return id_free_block;
}

// END: Filesystem creation of links in inode.
// ================================================================================================
