static int search_block_inodeid(void*, int32_t*, const int32_t*, size_t);
static int search_block_inodename(void*, int32_t*, const int32_t*, size_t);
static int search_blockid_diritems(void*, int32_t*, const int32_t*, size_t);
static int search_blockid_links(void*, int32_t*, const int32_t*, size_t);
static int clear_blocks(int32_t*, size_t);
static int clear_links(struct inode*);
static bool is_block_full_dirs(int32_t);
static bool is_block_full_links(int32_t);


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
		fs_read_directory_item(block, sb.count_dir_items);
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
		fs_read_directory_item(block, sb.count_dir_items);
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
			fs_read_directory_item(block, sb.count_dir_items);
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
			fs_read_int32t(block, sb.count_links);
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

static int clear_blocks(int32_t* links, const size_t size) {
	for (size_t i = 0; i < size; ++i) {
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
		fs_read_int32t(block_direct, sb.count_links);
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
		fs_read_int32t(block_indirect, sb.count_links);
		links_indirect = get_count_links(block_indirect);

		// loop over each 2nd level indirect link to get blocks with 1st level indirect links
		for (i = 0; i < links_indirect; ++i) {
			fs_seek_set(sb.addr_data + block_indirect[i] * sb.block_size);
			fs_read_int32t(block_direct, sb.count_links);
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

/*
 *  Check if block is full of directory items.
 *  Returns true if block is full, else false.
 */
static bool is_block_full_dirs(const int32_t id_block) {
	struct directory_item block[sb.count_dir_items];

	fs_seek_blocks(id_block);
	fs_read_directory_item(block, sb.count_dir_items);

	return (bool) (get_count_dirs(block) == sb.count_dir_items);
}

/*
 *  Check if block is full of links.
 *  Returns true if block is full, else false.
 */
static bool is_block_full_links(const int32_t id_block) {
	int32_t block[sb.count_links];

	fs_seek_blocks(id_block);
	fs_read_int32t(block, sb.count_links);

	return (bool) (get_count_links(block); == sb.count_links);
}
