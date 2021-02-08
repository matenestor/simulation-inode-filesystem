#include <memory.h>
#include <stddef.h>
#include <stdint.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "inode.h"
#include "iteration_carry.h"

#include "errors.h"


// ---------- ITERATE ---------------------------------------------------------

/*
 * Function checks every available link in given inode 'source'.
 * Start with direct links, and continue with indirect links level 1 and level 2.
 * For each block with direct links is called given function 'callback(...)',
 * where the block and struct carrying variables are passed as a parameter
 * needed by the callback function for processing.
 * To process all blocks, until some condition is met,
 * the 'callback(...)' must return 'false'.
 * This exists, because there are two types of inodes -- for file and for directory --
 * and various functions need to iterate links of inodes for their purposes.
 *
 * 'source' -- Inode, which is being checked.
 * 'foo' -- structure with variables needed by callback function
 * 'callback' -- pointer to function, which will be called for block searching
 */
int iterate_links(const struct inode* inode_source, void* carry, bool (*callback)()) {
	bool ret_call = false;
	size_t i, ii;
	uint32_t block_direct[sb.count_links];
	uint32_t block_indirect[sb.count_links];

	// mind the "&& !ret_call" in for loops

	// DIRECT LINKS
	ret_call = callback(inode_source->direct, COUNT_DIRECT_LINKS, carry);

	// INDIRECT LINKS LVL 1
	for (i = 0; i < COUNT_INDIRECT_LINKS_1 && !ret_call; ++i) {
		if (inode_source->indirect_1[i] == FREE_LINK)
			continue;
		fs_read_link(block_direct, sb.count_links, inode_source->indirect_1[i]);
		ret_call = callback(block_direct, sb.count_links, carry);
	}

	// INDIRECT LINKS LVL 2
	for (ii = 0; ii < COUNT_INDIRECT_LINKS_2 && !ret_call; ++ii) {
		if (inode_source->indirect_2[ii] == FREE_LINK)
			continue;

		fs_read_link(block_indirect, sb.count_links, inode_source->indirect_2[ii]);
		for (i = 0; i < sb.count_links && !ret_call; ++i) {
			if (block_indirect[i] == FREE_LINK)
				continue;
			fs_read_link(block_direct, sb.count_links, block_indirect[i]);
			ret_call = callback(block_direct, sb.count_links, carry);
		}
	}
	return ret_call ? RETURN_SUCCESS : RETURN_FAILURE;
}

// ---------- LINK FREE -------------------------------------------------------

/*
 * Free link by clearing block where given link points to
 * and free the block in bitmap. This invalidates the link.
 * Link must not be used further in function,
 * which calls this function and it should be set to FREE_LINK.
 */
static int free_link(const uint32_t id_block) {
	// NOTE possible optimization -- static char block[FS_BLOCK_SIZE] = {0};
	//  but FS_BLOCK_SIZE has to be visible here, now it is only in format.c
	char block[sb.block_size];
	memset(block, '\0', sb.block_size);
	fs_write_data(block, sb.block_size, id_block);
	free_bitmap_field_data(id_block);
	return RETURN_SUCCESS;
}

/*
 * Free all links in given array. See 'free_link(uint32_t)'.
 */
static int free_links_array(const uint32_t* links, const size_t count) {
	for (size_t i = 0; i < count; ++i) {
		if (links[i] == FREE_LINK)
			continue;
		free_link(links[i]);
	}
	return RETURN_SUCCESS;
}

static int free_links_direct(const uint32_t* links, const size_t count) {
	free_links_array(links, count);
	return RETURN_SUCCESS;
}

static int free_links_indirect_1(const uint32_t* links, const size_t count) {
	size_t i;
	uint32_t block_direct[sb.count_links];

	for (i = 0; i < count; i++) {
		if (links[i] != FREE_LINK) {
			// read whole block with direct links
			fs_read_link(block_direct, sb.count_links, links[i]);

			// free blocks where the direct links point to
			free_links_direct(block_direct, sb.count_links);

			// free block with direct links
			free_link(links[i]);
		}
	}
	return RETURN_SUCCESS;
}

static int free_links_indirect_2(const uint32_t* links, const size_t count) {
	size_t i;
	uint32_t block_indirect[sb.count_links];

	for (i = 0; i < count; ++i) {
		if (links[i] != FREE_LINK) {
			// read whole block with indirect links level 1
			fs_read_link(block_indirect, sb.count_links, links[i]);

			// free blocks where the indirect links level 1 point to
			free_links_indirect_1(block_indirect, sb.count_links);

			// free block with indirect links level 1
			free_link(links[i]);
		}
	}
	return RETURN_SUCCESS;
}

int free_all_links(struct inode* in_source) {
	size_t i;

	// free links of given inode
	free_links_direct(in_source->direct, COUNT_DIRECT_LINKS);
	free_links_indirect_1(in_source->indirect_1, COUNT_INDIRECT_LINKS_1);
	free_links_indirect_2(in_source->indirect_2, COUNT_INDIRECT_LINKS_2);

	// make links free in the inode itself
	for (i = 0; i < COUNT_DIRECT_LINKS; i++)
		in_source->direct[i] = FREE_LINK;
	for (i = 0; i < COUNT_INDIRECT_LINKS_1; i++)
		in_source->indirect_1[i] = FREE_LINK;
	for (i = 0; i < COUNT_INDIRECT_LINKS_2; i++)
		in_source->indirect_2[i] = FREE_LINK;

	return RETURN_SUCCESS;
}

/*
 * Reset links created by 'create_empty_links()', if error happened there.
 */
static int reset_created_links(uint32_t* links_tmp, const uint32_t* links_source, const size_t count) {
	for (size_t i = 0; i < count; ++i) {
		if (links_tmp[i] != links_source[i]) {
			free_links_direct(&links_tmp[i], count);
			links_tmp[i] = FREE_LINK;
		}
	}
	return RETURN_SUCCESS;
}

// ---------- LINK CREATE -----------------------------------------------------

/*
 * Initialize new link to empty data block.
 * Return index number of the block, or 'RETURN_FAILURE'.
 */
static uint32_t init_link_() {
	return allocate_bitmap_field_data();
}

/*
 * Initialize block with 'FREE_LINK's and first link to empty block.
 * Return index number of the block, or 'RETURN_FAILURE'.
 */
static uint32_t init_block_with_links_(const uint32_t id_block) {
	uint32_t free_id = FREE_LINK;
	uint32_t block[sb.count_links];

	if ((free_id = init_link_()) != FREE_LINK) {
		memset(block, FREE_LINK, sb.count_links);
		block[0] = free_id;
		fs_write_link(block, sb.count_links, id_block);
	}

	return free_id;
}

/*
 * Create new direct link.
 * Returns index number of block where new link points to, or 'RETURN_FAILURE'.
 */
static uint32_t create_direct() {
	return init_link_();
}

/*
 * Create new indirect link level 1. Argument is place, where start of the link will be saved.
 * 		'root_link' --> |block_w_direct_links| --> 'id_block_data'
 * Returns index number of block where new link points to, or 'RETURN_FAILURE'.
 */
static int32_t create_indirect_1(uint32_t* root_link) {
	uint32_t id_block_direct = FREE_LINK;
	uint32_t id_block_data = FREE_LINK;

	// create link for data block with direct links
	if ((id_block_direct = create_direct()) != FREE_LINK) {
		// after the block init at 'id_block_direct', link to free data block
		// from inside the initialized data block is returned
		if ((id_block_data = init_block_with_links_(id_block_direct)) != FREE_LINK) {
			// id of indirect link lvl 1 is assigned to given parameter
			*root_link = id_block_direct;
		}
			// block was not initialized == no more free blocks
		else {
			free_bitmap_field_data(id_block_direct);
		}
	}
	return id_block_data;
}

/*
 * Create new indirect link level 2. Argument is place, where start of the link will be saved.
 * 		'root_link' --> |block_w_indirect_links_1| --> |block_w_direct_links| --> 'id_block_data'
 * Returns index number of block where new link points to, or 'RETURN_FAILURE'.
 */
static uint32_t create_indirect_2(uint32_t* root_link) {
	uint32_t id_block_indirect = FREE_LINK;
	uint32_t id_block_direct = FREE_LINK;
	uint32_t id_block_data = FREE_LINK;

	// create link for data block with indirect links lvl 1
	// 'id_block_indirect' is assigned id of link to data block with indirect links lvl 1
	// and returned 'id_block_direct' is id for next block with links (direct links)
	if ((id_block_direct = create_indirect_1(&id_block_indirect)) != FREE_LINK) {
		if ((id_block_data = init_block_with_links_(id_block_direct)) != FREE_LINK) {
			// id of indirect link lvl 2 is assigned to given parameter
			*root_link = id_block_indirect;
		}
			// blocks were not initialized == no more free blocks, so clear block with indirect links
			// level 1 and turn on both blocks with indirect links level 1 and direct links again
		else {
			free_bitmap_field_data(id_block_direct);
			free_link(id_block_indirect);
		}
	}
	return id_block_data;
}

static int create_direct_links(uint32_t** buffer, size_t* created, const size_t to_create,
							   uint32_t* direct_links, const size_t links_count) {
	size_t i;
	uint32_t tmp_link = FREE_LINK;

	for (i = 0; i < links_count && *created < to_create; ++i) {
		if (direct_links[i] == FREE_LINK) {
			if ((tmp_link = create_direct()) == FREE_LINK) {
				break; // error
			}

			direct_links[i] = tmp_link;
			**buffer = tmp_link;
			(*buffer)++;
			(*created)++;
		}
	}
	return *created == to_create ? RETURN_SUCCESS : RETURN_FAILURE;
}

static int create_indirect_links_1(uint32_t** buffer, size_t* created, const size_t to_create,
								   uint32_t* indirect_links_1, const size_t links_count) {
	size_t i;
	uint32_t tmp_link = FREE_LINK;
	uint32_t direct_links[sb.count_links];

	for (i = 0; i < links_count && *created < to_create; ++i) {
		if (indirect_links_1[i] == FREE_LINK) {
			// for 'create_indirect_1()', start of the indirect link lvl 1 is given as argument,
			// and set inside the function, and end of the link is returned
			if ((tmp_link = create_indirect_1(&indirect_links_1[i])) == FREE_LINK) {
				break; // error
			}

			**buffer = tmp_link;
			(*buffer)++;
			(*created)++;
		}

		fs_read_link(direct_links, sb.count_links, indirect_links_1[i]);
		if (create_direct_links(buffer, created, to_create,
								direct_links, sb.count_links) == RETURN_FAILURE) {
			break; // error
		}
		fs_write_link(direct_links, sb.count_links, indirect_links_1[i]);
	}

	return *created == to_create ? RETURN_SUCCESS : RETURN_FAILURE;
}

static int create_indirect_links_2(uint32_t** buffer, size_t* created, const size_t to_create,
								   uint32_t* indirect_links_2, const size_t links_count) {
	size_t i;
	uint32_t tmp_link = FREE_LINK;
	uint32_t indirect_links_1[sb.count_links];

	for (i = 0; i < links_count && *created < to_create; ++i) {
		if (indirect_links_2[i] == FREE_LINK) {
			// for 'create_indirect_2()', start of the indirect link lvl 2 is given as argument,
			// and set inside the function, and end of the link is returned
			if ((tmp_link = create_indirect_2(&indirect_links_2[i])) == FREE_LINK) {
				break; // error
			}

			**buffer = tmp_link;
			(*buffer)++;
			(*created)++;
		}

		fs_read_link(indirect_links_1, sb.count_links, indirect_links_2[i]);
		if (create_indirect_links_1(buffer, created, to_create,
									indirect_links_1, sb.count_links) == RETURN_FAILURE) {
			break; // error
		}
		fs_write_link(indirect_links_1, sb.count_links, indirect_links_2[i]);
	}

	return *created == to_create ? RETURN_SUCCESS : RETURN_FAILURE;
}

/*
 * Create 'count' of links in given inode. Creation is done always on 'FREE_LINK'
 * both in inode itself and in data blocks with indirect links.
 * If 'count' of links is successfully created, they are put into given inode,
 * else the links are freed.
 *
 * buffer -- to be filled with created direct link ids
 * count -- amount of direct links to be allocated
 * in_source -- inode to allocate links in
*/
int create_empty_links(uint32_t* buffer, const size_t to_create, struct inode* inode_source) {
	size_t created = 0;
	struct inode inode_tmp;
	memcpy(&inode_tmp, inode_source, sizeof(struct inode));

	if (create_direct_links(&buffer, &created, to_create,
							inode_tmp.direct, COUNT_DIRECT_LINKS) == RETURN_FAILURE) {
		goto free_direct;
	}
	if (create_indirect_links_1(&buffer, &created, to_create,
								inode_tmp.indirect_1, COUNT_INDIRECT_LINKS_1) == RETURN_FAILURE) {
		goto free_indirect_1;
	}
	if (create_indirect_links_2(&buffer, &created, to_create,
								inode_tmp.indirect_2, COUNT_INDIRECT_LINKS_2) == RETURN_FAILURE) {
		goto free_indirect_2;
	}

	// successfully initialized 'to_create' of new links
	if (created == to_create) {
		// total usage of space of directory inode is increased here
		// and calculated are only leafs of links -- no deep blocks with indirect links
		if (inode_source->inode_type == Inode_type_dirc) {
			inode_source->file_size += (to_create * sb.block_size);
		}
		memcpy(inode_source, &inode_tmp, sizeof(struct inode));
		fs_write_inode(&inode_tmp, 1, inode_tmp.id_inode);
		return RETURN_SUCCESS;
	}

	// Most of the time, there will be enough space for allocation,
	// so it is faster to allocate immediately and in case free the links again,
	// than checking at first, if there is enough data blocks for allocation.

	// error during allocating links, so destroy all created links so far
	// from temporary inode == source inode is untouched, but there were write to filesystem
free_indirect_2:
	reset_created_links(inode_tmp.indirect_2, inode_source->indirect_2, COUNT_INDIRECT_LINKS_2);
free_indirect_1:
	reset_created_links(inode_tmp.indirect_1, inode_source->indirect_1, COUNT_INDIRECT_LINKS_1);
free_direct:
	reset_created_links(inode_tmp.direct, inode_source->direct, COUNT_DIRECT_LINKS);
	// clear also given 'buffer' array
	for (size_t i = 0; i < created; i++) {
		buffer--;
		*buffer = FREE_LINK;
	}
	// TODO reset also deep links -- blocks with indirect links in filesystem
	set_myerrno(Err_inode_no_links);
	return RETURN_FAILURE;
}
