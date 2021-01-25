#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "fs_io.h"
#include "fs_cache.h"


void fs_seek_set(uint32_t offset) {
	if (offset > INT32_MAX) {
		// this filesystem has max of 4 GB, so there is no need
		// to have a loop for decreasing the offset and fseeking by steps
		offset -= INT32_MAX;
		fseek(filesystem, INT32_MAX, SEEK_SET);
		fseek(filesystem, offset, SEEK_CUR);
	}
	else {
		fseek(filesystem, offset, SEEK_SET);
	}
}

void fs_flush() {
	fflush(filesystem);
}

unsigned int fs_read_superblock(struct superblock* buffer, const size_t count) {
	return fread(buffer, sizeof(struct superblock), count, filesystem);
}

unsigned int fs_read_inode(struct inode* buffer, const size_t count) {
	return fread(buffer, sizeof(struct inode), count, filesystem);
}

unsigned int fs_read_directory_item(struct directory_item* buffer, const size_t count) {
	return fread(buffer, sizeof(struct directory_item), count, filesystem);
}

unsigned int fs_read_int32t(int32_t* buffer, const size_t count) {
	return fread(buffer, sizeof(int32_t), count, filesystem);
}

unsigned int fs_read_bool(bool* buffer, const size_t count) {
	return fread(buffer, sizeof(bool), count, filesystem);
}

unsigned int fs_read_char(char* buffer, const size_t count) {
	return fread(buffer, sizeof(char), count, filesystem);
}

unsigned int fs_write_superblock(const struct superblock* buffer, const size_t count) {
	return fwrite(buffer, sizeof(struct superblock), count, filesystem);
}

unsigned int fs_write_inode(const struct inode* buffer, const size_t count) {
	return fwrite(buffer, sizeof(struct inode), count, filesystem);
}

unsigned int fs_write_directory_item(const struct directory_item* buffer, const size_t count) {
	return fwrite(buffer, sizeof(struct directory_item), count, filesystem);
}

unsigned int fs_write_int32t(const int32_t* buffer, const size_t count) {
	return fwrite(buffer, sizeof(int32_t), count, filesystem);
}

unsigned int fs_write_bool(const bool* buffer, const size_t count) {
	return fwrite(buffer, sizeof(bool), count, filesystem);
}

unsigned int fs_write_char(const char* buffer, const size_t count) {
	return fwrite(buffer, sizeof(char), count, filesystem);
}