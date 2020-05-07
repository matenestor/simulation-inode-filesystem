#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "inc/fs_cache.h"
#include "inc/fs_prompt.h"
#include "inc/return_codes.h"

#include "inc/logger_api.h"
#include "error.h"


/******************************************************************************
 *
 *  Remove separators of directories from end of given path.
 *
 */
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


/******************************************************************************
 *
 *  Parse name of last directory in given path.
 *
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


/******************************************************************************
 *
 *  Parse path to last element in given path. Everything except the last element.
 *
 */
int parse_parent_path(char* parent_path, const char* path) {
    int ret = RETURN_FAILURE;
    char* last_sep = NULL;
    char buff[strlen(path) + 1];

    if (strlen(path) > 0) {
        strncpy(buff, path, strlen(path) + 1);

        // remove SEPARATORs from the end of the path
        remove_end_separators(buff);

        // find last element in given path
        last_sep = strrchr(path, SEPARATOR[0]);

        // in path there is SEPARATOR
        if (last_sep != NULL) {
            strncpy(parent_path, path, last_sep - path);
            parent_path[last_sep - path] = '\0';

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


/******************************************************************************
 *
 *  Get count of links in indirect links cluster.
 *
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


/******************************************************************************
 *
 *  Get count of directory records in directory cluster.
 *
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
