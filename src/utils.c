#include <string.h>

#include "utils.h"
#include "fs_cache.h"
#include "fs_prompt.h"

#include "../include/logger.h"
#include "../include/errors.h"


int remove_end_separators(char* path) {
	char* p_path = NULL;

	if (strlen(path) > 1) {
		p_path = path + strlen(path) - 1;

		// remove all SEPARATORs in the end
		while (*p_path == SEPARATOR[0]) {
			*p_path = '\0';
			--p_path;
		}
	}
	return 0;
}

/*
 *  Parse name of last directory in given path.
 */
int parse_name(char* name, const char* path, const size_t length) {
	int ret = RETURN_FAILURE;
	char* last_sep = NULL;
	char buff[strlen(path) + 1];

	if (strlen(path) > 0) {
		strncpy(buff, path, strlen(path) + 1);

		// remove SEPARATORs from the end of the path
		remove_end_separators(buff);
		// find last element in given path
		last_sep = strrchr(path, SEPARATOR[0]);
		// in case there is no SEPARATOR in path -- name is without path (eg. just "etc")
		last_sep = last_sep == NULL ? buff : last_sep + 1;

		// check if given name is not too long
		if (strlen(last_sep) < length) {
			strncpy(name, last_sep, strlen(last_sep) + 1);

			ret = RETURN_SUCCESS;

			log_info("Parsed name [%s].", name);
		}
		else {
			set_myerrno(Err_item_name_long);

			log_warning("Name is too long for parsing.");
		}
	}

	return ret;
}

/*
 *  Parse path to last element in given path. Everything except the last element.
 */
int parse_parent_path(char* parent_path, const char* path) {
	int ret = RETURN_FAILURE;
	char* last_sep = NULL;
	char buff[strlen(path) + 1];
	size_t len = 0;

	if (strlen(path) > 0) {
		strncpy(buff, path, strlen(path) + 1);

		// remove SEPARATORs from the end of the path
		remove_end_separators(buff);
		// find last element in given path
		last_sep = strrchr(path, SEPARATOR[0]);

		// in path there is SEPARATOR
		if (last_sep != NULL) {
			len = last_sep - path;
			len = len == 0 ? len + 1 : len;

			strncpy(parent_path, path, len);
			parent_path[len] = '\0';

			ret = RETURN_SUCCESS;

			log_info("Parsed parent path [%s].", parent_path);
		}
		// no parent path given (eg. just "etc", not "/etc")
		else {
			*parent_path = '\0';
		}
	}

	return ret;
}

/*
 *  Get count of links in indirect links block.
 */
size_t get_count_links(int32_t* source) {
	size_t items = 0;
	int32_t* p_link = source;

	while (*p_link != FREE_LINK && items < sb.count_links) {
		++p_link;
		++items;
	}

	return items;
}

/*
 *  Get count of directory records in directory block.
 */
size_t get_count_dirs(struct directory_item* source) {
	size_t items = 0;
	struct directory_item* p_dir = source;

	while (strcmp(p_dir->item_name, "") != 0 && items < sb.count_dir_items) {
		++p_dir;
		++items;
	}

	return items;
}
