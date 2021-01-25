#ifndef FS_BITMAP_H
#define FS_BITMAP_H

#include <stdint.h>

void bitmap_field_inodes_on(int32_t);
void bitmap_field_inodes_off(int32_t);
void bitmap_field_blocks_on(int32_t);
void bitmap_field_blocks_off(int32_t);
int32_t get_empty_bitmap_field(int32_t);

#endif
