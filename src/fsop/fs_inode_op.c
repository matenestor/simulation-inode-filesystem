#include <stdint.h>
#include <string.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "iteration_carry.h"

#include "errors.h"
#include "logger.h"


// ---------- INODE FREE ------------------------------------------------------

/*
 * 	Free given inode by deleting record from its parent's blocks,
 * 	resetting its values, freeing links and turning on its bitmap field.
 */
static int free_inode(struct inode* inode_delete, const uint32_t id_parent) {
	struct carry_directory_item carry = {inode_delete->id_inode, ""};
	struct inode inode_parent = {0};

	// delete record from parent
	fs_read_inode(&inode_parent, 1, id_parent);
	iterate_links(&inode_parent, &carry, delete_block_item);

	// free given inode
	inode_delete->inode_type = Inode_type_free;
	inode_delete->file_size = 0;
	free_all_links(inode_delete);
	free_bitmap_field_inode(inode_delete->id_inode);
	fs_write_inode(inode_delete, 1, inode_delete->id_inode);

	return RETURN_SUCCESS;
}

/*
 * Free file inode. If 'id_inode' is id of directory or free inode, error is set.
 */
int free_inode_file(const uint32_t id_inode, const uint32_t id_parent) {
	int ret = RETURN_FAILURE;
	struct inode inode_delete = {0};

	fs_read_inode(&inode_delete, 1, id_inode);

	if (inode_delete.inode_type == Inode_type_file) {
		free_inode(&inode_delete, id_parent);
		ret = RETURN_SUCCESS;
	}
	else if (inode_delete.inode_type == Inode_type_dirc) {
		set_myerrno(Err_item_not_file);
	} else {
		set_myerrno(Err_item_not_exists);
	}
	return ret;
}

/*
 * Free file inode. If directory is not empty
 * or 'id_inode' is id of file or free inode, error is set.
 */
int free_inode_directory(const uint32_t id_inode, const uint32_t id_parent) {
	int ret = RETURN_FAILURE;
	struct inode inode_delete = {0};

	fs_read_inode(&inode_delete, 1, id_inode);

	if (inode_delete.inode_type == Inode_type_dirc) {
		if (is_directory_empty(&inode_delete)) {
			free_inode(&inode_delete, id_parent);
			ret = RETURN_SUCCESS;
		} else {
			set_myerrno(Err_dir_not_empty);
		}
	}
	else if (inode_delete.inode_type == Inode_type_file) {
		set_myerrno(Err_item_not_directory);
	} else {
		set_myerrno(Err_item_not_exists);
	}
	return ret;
}

// ---------- INODE CREATE ----------------------------------------------------

/*
 *  Create new inode for a file. One inode from filesystem is used.
 *  Returns index number of new inode, or 'RETURN_FAILURE'.
 */
uint32_t create_inode_file(struct inode* new_inode) {
	// id of new inode, which will be used
	uint32_t id_free_inode = allocate_bitmap_field_inode();

	if (id_free_inode != FREE_LINK) {
		// cache the free inode, init it and write it
		fs_read_inode(new_inode, 1, id_free_inode);
		new_inode->inode_type = Inode_type_file;
		fs_write_inode(new_inode, 1, id_free_inode);

		log_info("New file inode created, id: [%d].", id_free_inode);
	} else {
		// no need for free_bitmap_field_inode(), because nothing
		// was allocated, if 'id_free_inode' is RETURN_FAILURE
		my_perror("filesystem");
		set_myerrno(Err_inode_no_inodes);

		log_error("Unable to create new inode.");
	}

	return id_free_inode;
}

/*
 *  Create new inode for a directory. One inode and one data block from filesystem is used.
 *  Returns index number of new inode, or 'RETURN_FAILURE'.
 */
uint32_t create_inode_directory(struct inode* new_inode, const uint32_t id_parent) {
	// id of new inode, which will be initialized
	uint32_t id_free_inode = allocate_bitmap_field_inode();
	// id of new block, where first direct link will point to; equal to create_direct()
	uint32_t id_free_block = allocate_bitmap_field_data();
	// default folders in each directory
	struct directory_item new_dirs[sb.count_dir_items];

	// if there is free inode and data block for it, use it
	if (!(id_free_inode == FREE_LINK || id_free_block == FREE_LINK)) {
		fs_read_inode(new_inode, 1, id_free_inode);

		// init first block of new directory
		init_block_with_directories(new_dirs);
		new_dirs[0].fk_id_inode = id_free_inode;
		new_dirs[1].fk_id_inode = id_parent;
		strncpy(new_dirs[0].item_name, ".", 1);
		strncpy(new_dirs[1].item_name, "..", 2);

		// init new inode
		new_inode->inode_type = Inode_type_dirc;
		new_inode->direct[0] = id_free_block;

		// write new updated inode and data block
		fs_write_inode(new_inode, 1, id_free_inode);
		fs_write_directory_item(new_dirs, sb.count_dir_items, id_free_block);

		log_info("New directory inode created, id: [%d].", id_free_inode);
	}
	else {
		my_perror("filesystem");
		// getting empty bitmap fields turned them off, so turn them on again
		if (id_free_inode != FREE_LINK) {
			id_free_inode = FREE_LINK;
			free_bitmap_field_inode(id_free_inode);
			set_myerrno(Err_inode_no_inodes);
		}
		if (id_free_block != FREE_LINK) {
			free_bitmap_field_data(id_free_block);
			set_myerrno(Err_block_no_blocks);
		}

		log_error("Unable to create new inode.");
	}

	return id_free_inode;
}
