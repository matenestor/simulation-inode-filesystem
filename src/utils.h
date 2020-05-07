#ifndef UTILS_H
#define UTILS_H


#include <stdint.h>

#include "inc/inode.h"


int remove_end_separators(char*);
int parse_name(char*, const char*, size_t);
int parse_parent_path(char*, const char*);
size_t get_count_links(int32_t*);
size_t get_count_dirs(struct directory_item*);


#endif
