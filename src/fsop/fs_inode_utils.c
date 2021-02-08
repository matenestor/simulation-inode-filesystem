#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "fs_api.h"
#include "fs_cache.h"
#include "fs_prompt.h"
#include "iteration_carry.h"

#include "errors.h"
#include "logger.h"


/*
 * Get id of parent of given inode, which must be directory.
 */
static int32_t get_parent_inode_id(struct inode* inode_child) {
	assert(inode_child->inode_type == Inode_type_dirc);

	struct directory_item dot_dirs[2] = {0};
	fs_read_directory_item(dot_dirs, 2, inode_child->direct[0]);
	// it would be possible to iterate all blocks of inode, until ".." directory was found,
	// but this is just faster and by filesystem definition, it will be always here,
	// unless you touch raw byte data
	return dot_dirs[1].id_inode;
}

/*
 *  Get id of inode from given path and id of its parent. If path starts with "/",
 *  then function searches from root inode, else from actual inode, where user is.
 *  Inodes, to the final one, are read and traversed folder by folder in path.
 */
static int get_inode_ids(uint32_t* p_id_destination, uint32_t* p_id_parent, const char* path) {
	int ret_iter = RETURN_FAILURE;
	uint32_t id_dest = 0;
	uint32_t id_parent = 0;
	char* dir = NULL;
	char path_copy[strlen(path) + 1];
	struct inode inode_dest = {0};
	struct carry_dir_item carry = {0};

	strncpy(path_copy, path, strlen(path) + 1);

	if (path_copy[0] == SEPARATOR[0]) {
		// track path from root
		fs_read_inode(&inode_dest, 1, ROOT_ID);
	} else {
		// track path from actual directory
		fs_read_inode(&inode_dest, 1, in_actual.id_inode);
	}
	// init very first ids of inodes in path
	id_dest = inode_dest.id_inode;
	id_parent = get_parent_inode_id(&inode_dest);

	dir = strtok(path_copy, SEPARATOR);
	// getting root inode -- no while loop
	if (dir == NULL)
		ret_iter = RETURN_SUCCESS;

	// go over all elements in given path
	while (dir != NULL) {
		// get inode of element in given path
		strncpy(carry.name, dir, strlen(dir));
		ret_iter = iterate_links(&inode_dest, &carry, search_block_inode_id);

		// element in path was found in block
		if (ret_iter != RETURN_FAILURE) {
			id_parent = id_dest;
			id_dest = carry.id;
			fs_read_inode(&inode_dest, 1, id_dest);
		} else {
			// no item with parsed name found
			id_parent = FREE_LINK;
			id_dest = FREE_LINK;
			set_myerrno(Err_item_not_exists);
			log_error("Item does not exist [%s]\n", path);
			break;
		}

		dir = strtok(NULL, SEPARATOR);
	}
	*p_id_destination = id_dest;
	*p_id_parent = id_parent;

	return ret_iter;
}

/*
 *  Get inode from given path.
 */
int get_inode(struct inode* inode_dest, const char* path) {
	uint32_t id_destination = FREE_LINK;
	uint32_t id_parent = FREE_LINK;
	if (get_inode_ids(&id_destination, &id_parent, path) != RETURN_FAILURE) {
		fs_read_inode(inode_dest, 1, id_destination);
		return RETURN_SUCCESS;
	}
	inode_dest->inode_type = Inode_type_free;
	return RETURN_FAILURE;
}

/*
 *  Get inode and its parent from given path.
 */
int get_inode_wparent(struct inode* inode_dest, struct inode* inode_parent, const char* path) {
	uint32_t id_destination = FREE_LINK;
	uint32_t id_parent = FREE_LINK;
	if (get_inode_ids(&id_destination, &id_parent, path) != RETURN_FAILURE) {
		fs_read_inode(inode_dest, 1, id_destination);
		fs_read_inode(inode_parent, 1, id_parent);
		return RETURN_SUCCESS;
	}
	inode_dest->inode_type = Inode_type_free;
	return RETURN_FAILURE;
}

static int handle_child_name(char** p_buffer, int* size_remaining, const uint32_t id_inode,
							 const char* child_name, const size_t length_child_name) {
	// write child name and separator to path
	*size_remaining -= (int) (length_child_name + 1);
	// enough space in buffer OR size is completely used, but current inode is root
	if (*size_remaining > 2 || (*size_remaining >= 0 && id_inode == 1)) {
		// move pointer before child name and write the name
		*p_buffer -= length_child_name;
		strncpy(*p_buffer, child_name, length_child_name);
		// write separator before the name
		*p_buffer -= 1;
		**p_buffer = SEPARATOR[0];
		return RETURN_SUCCESS;
	}
	// path to root is too long
	else {
		*p_buffer -= 2;
		strncpy(*p_buffer, "..", 2);
		return RETURN_FAILURE;
	}
}

/*
 *  Get path from root to actual inode by going back from it to root over parents.
 *  If path would be too long for pwd, ".." is written at the beginning.
 */
int get_path_to_root(char* dest_path, const size_t length_path, const struct inode* inode_source) {
	struct carry_dir_item carry = {0};
	struct inode inode_tmp = {0};
	uint32_t id_parent = FREE_LINK;
	int size_remaining = length_path;
	char child_name[STRLEN_ITEM_NAME] = {0};	// buffer for names of directories on path to root
	size_t length_child_name = 0;
	char buffer[length_path];					// buffer for whole path to root
	char* p_buffer = buffer + length_path - 2;	// pointer to end of buffer

	// in case 'inode_source' is root already, write root to path
	*p_buffer = SEPARATOR[0];

	memcpy(&inode_tmp, inode_source, sizeof(struct inode));
	memset(buffer, '\0', length_path);

	// while not in root inode
	while (inode_tmp.id_inode != ROOT_ID) {
		// prepare child id for search of its name
		carry.id = inode_tmp.id_inode;
		// get parent inode
		id_parent = get_parent_inode_id(&inode_tmp);
		fs_read_inode(&inode_tmp, 1, id_parent);

		// get name of child inode
		if (iterate_links(&inode_tmp, &carry, search_block_inode_name) == RETURN_FAILURE) {
			// child inode is probably lost
			set_myerrno(Err_fs_error);
			goto fail;
		}

		// copy child inode name from carry
		length_child_name = strlen(carry.name);
		strncpy(child_name, carry.name, length_child_name);

		if (handle_child_name(&p_buffer, &size_remaining, inode_tmp.id_inode,
						  child_name, length_child_name) == RETURN_FAILURE) {
			break; // buffer size limit reached
		}
	}

	// copy path to root to given buffer
	strncpy(dest_path, p_buffer, length_path);
	return RETURN_SUCCESS;

fail:
	strncpy(dest_path, "unknown_path", 13);
	return RETURN_FAILURE;
}

/*
 * Add record of new inode to its parent directory inode.
 */
int add_to_parent(struct inode* inode_parent, struct carry_dir_item* carry) {
	// link number to empty block, in case all blocks of parent are full
	uint32_t empty_block[1] = {0};

	// add record to parent inode about new directory
	if (iterate_links(inode_parent, carry, add_block_item) == RETURN_FAILURE) {
		// parent inode has all, so far created, blocks full
		if (create_empty_links(empty_block, 1, inode_parent) != RETURN_FAILURE) {
			init_block_with_directories(empty_block[0]);
			add_block_item(empty_block, 1, &carry);
		} else {
			// error while creating new link, or parent inode is completely full of directories
			return RETURN_FAILURE;
		}
	}
	return RETURN_SUCCESS;
}

/*
 * Check if directory, represented by given inode, is empty.
 */
bool is_directory_empty(const struct inode* inode_source) {
	// if directory doesn't have common directories,
	// meaning other than "." and "..", then it is empty
	return (bool) (iterate_links(inode_source, NULL, has_common_directories) == RETURN_FAILURE);
}

/*
 * 	Check if making new directory is possible.
 */
bool item_exists(const struct inode* inode_parent, const char* dir_name) {
	bool exists = false;
	struct carry_dir_item carry = {0};

	carry.id = FREE_LINK;
	strncpy(carry.name, dir_name, STRLEN_ITEM_NAME);

	// check if item already exists
	if (iterate_links(inode_parent, &carry, search_block_inode_id) != RETURN_FAILURE) {
		exists = true;
	}
	return exists;
}
