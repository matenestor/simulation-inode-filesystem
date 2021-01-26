static int search_links(void*, int32_t*, const struct inode*, int (*)(void*, int32_t*, const int32_t*, const size_t));
static int32_t init_link_();
static int32_t init_block_(int32_t);
static int32_t clear_block_(int32_t);
static int32_t create_direct();
static int32_t create_indirect_1(int32_t*);
static int32_t create_indirect_2(int32_t*);


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
		fs_read_int32t(block_direct, sb.count_links);
		links_direct = get_count_links(block_direct);

		ret = search_block(name, id, block_direct, links_direct);
	}

	// third -- check indirect links of 2nd level
	// note: if count of indirect links lvl 2 needs to be bigger than 1 in future, this `if` is needed to be in loop
	if (source->indirect2[0] != FREE_LINK && ret == RETURN_FAILURE) {
		// read whole block with 2nd level indirect links to blocks with 1st level indirect links
		fs_seek_set(sb.addr_data + source->indirect2[0] * sb.block_size);
		fs_read_int32t(block_indirect, sb.count_links);
		links_indirect = get_count_links(block_indirect);

		// loop over each 2nd level indirect link to get blocks with 1st level indirect links
		for (i = 0; i < links_indirect; ++i) {
			fs_seek_set(sb.addr_data + block_indirect[i] * sb.block_size);
			fs_read_int32t(block_direct, sb.count_links);
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
		fs_write_int32t(block, sb.count_links);
		fs_flush();
	}

	return free_index;
}

static int32_t clear_block_(int32_t id_block) {
	char block[sb.block_size];
	memset(block, '\0', sb.block_size);

	// write initialized block
	fs_seek_set(sb.addr_data + id_block * sb.block_size);
	fs_write_char(block, sb.block_size);
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
		fs_read_inode(new_inode, 1);

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
			fs_write_directory_item(new_dir_item, 2);

			fs_flush();
		}

		// write new updated inode
		fs_seek_set(sb.addr_inodes + id_free_inode * sizeof(struct inode));
		fs_write_inode(new_inode, 1);

		fs_flush();

		ret = id_free_inode;

		log_info("New inode created, type: [%s], id: [%d].", type == Itemtype_directory ? "directory" : "file", id_free_inode);
	}
	else {
		log_error("Unable to create new inode.");
	}

	return ret;
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
		fs_read_directory_item(block, sb.count_dir_items);

		// ".." directory is on second place in records
		id_parent = block[1].fk_id_inode;

		// read parent inode
		fs_seek_set(sb.addr_inodes + id_parent * sizeof(struct inode));
		fs_read_inode(&in_parent, 1);

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
	fs_write_inode(old_inode, 1);

	// turn on its bitmap field
	bitmap_field_on(sb.addr_bm_inodes, old_inode->id_inode);

	return 0;
}

/*
 *  Get first available link in given inode, or create new link in the inode.
 *  Returns id of block where the link points to, or 'RETURN_FAILURE'.
 */
// TODO perhaps, get COUNT of links would be better
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

	// TODO indirect1
	// TODO indirect2

	// if new link was created, write the change to filesystem
	if (is_new_link) {
		fs_seek_set(sb.addr_inodes + source->id_inode * sizeof(struct inode));
		fs_write_inode(source, 1);
		fs_flush();
	}

	return id_free_block;
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
		fs_read_inode(in_dest, 1);
	}
	// track path from actual directory
	else {
		fs_seek_set(sb.addr_inodes + in_actual.id_inode * sizeof(struct inode));
		fs_read_inode(in_dest, 1);
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
			fs_read_inode(in_dest, 1);
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
		fs_read_inode(&in_tmp, 1);

		// get id of parent inode
		search_links("..", &id_parent, &in_tmp, search_block_inodeid);

		// cache parent inode
		fs_seek_set(sb.addr_inodes + id_parent * sizeof(struct inode));
		fs_read_inode(&in_tmp, 1);

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

// ================================================================================================

int delete_empty_links(int32_t* link, struct inode* source) {
	size_t i, items = 0;
	int32_t id_block, id_block2;
	int32_t block[sb.count_dir_items];

	// search for id of block, where link to empty block is located
	search_links(&id_block, link, source, search_blockid_links);

	fs_seek_set(sb.addr_data + id_block * sb.block_size);
	fs_read_int32t(block, sb.count_links);
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
	fs_read_directory_item(block, sb.count_dir_items);
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
