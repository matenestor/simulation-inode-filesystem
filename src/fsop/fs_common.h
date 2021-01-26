#ifndef FS_OPERATIONS_H
#define FS_OPERATIONS_H

#include <stdbool.h>

void init_filesystem(const char*, bool*);
void close_filesystem();

#endif
